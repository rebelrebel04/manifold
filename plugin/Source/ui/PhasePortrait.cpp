#include "PhasePortrait.h"
#include "../Params.h"

namespace manifold::ui
{

namespace
{
    struct FilterPalette
    {
        float trailR, trailG, trailB;
        float beaconR, beaconG, beaconB;
        juce::Colour label;
    };

    FilterPalette paletteFor (manifold::params::FilterType t) noexcept
    {
        using FT = manifold::params::FilterType;
        switch (t)
        {
            case FT::MoogLadder:  return { 2.6f, 1.2f, 0.3f,  3.0f, 1.6f, 0.5f, juce::Colour (0xffffb870) };
            case FT::DiodeLadder: return { 0.5f, 2.4f, 0.7f,  0.8f, 2.8f, 1.0f, juce::Colour (0xff8effa0) };
            case FT::TunedComb:   return { 0.3f, 1.8f, 2.6f,  0.5f, 2.2f, 3.0f, juce::Colour (0xff7ad6ff) };
            case FT::SVF:
            default:              return { 1.4f, 0.78f, 2.6f, 1.8f, 1.1f, 3.0f, juce::Colour (0xffb59cff) };
        }
    }

    manifold::params::FilterType readFilterType (juce::AudioProcessorValueTreeState& apvts) noexcept
    {
        return (manifold::params::FilterType) (int) apvts.getRawParameterValue (manifold::params::id::filterType)->load();
    }
}

namespace gl = juce::gl;

namespace
{

constexpr const char* kTrailVS = R"(
#version 150 core
in vec2 position;
in float age;
in float u;
out float vAge;
out float vU;
void main()
{
    vAge = age;
    vU = u;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

// Color premultiplied so additive blending accumulates straight into the FBO.
// vU is [-1, +1] across the ribbon — gives soft edge falloff.
constexpr const char* kTrailFS = R"(
#version 150 core
in float vAge;
in float vU;
out vec4 fragColor;
uniform vec3 trailColor;
void main()
{
    float ageA  = vAge * vAge;
    float edge  = 1.0 - vU * vU;
    float crossA = pow(max(edge, 0.0), 1.4);
    float a = ageA * crossA;
    fragColor = vec4(trailColor * a, a);
}
)";

// Point sprite beacon — pixel size uniform via gl_PointSize, radial alpha falloff in FS.
constexpr const char* kBeaconVS = R"(
#version 150 core
in vec2 position;
in float size;
in float intensity;
out float vIntensity;
void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    gl_PointSize = size;
    vIntensity = intensity;
}
)";

constexpr const char* kBeaconFS = R"(
#version 150 core
in float vIntensity;
out vec4 fragColor;
uniform vec3 beaconColor;
void main()
{
    vec2 c = gl_PointCoord - vec2(0.5);
    float d = length(c) * 2.0;
    float a = pow(max(1.0 - d, 0.0), 1.6) * vIntensity;
    fragColor = vec4(beaconColor * a, a);
}
)";

constexpr const char* kQuadVS = R"(
#version 150 core
in vec2 position;
out vec2 vUv;
void main()
{
    vUv = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

// 9-tap gaussian (sigma ~3) — direction supplied via texelStep uniform.
constexpr const char* kBlurFS = R"(
#version 150 core
in vec2 vUv;
out vec4 fragColor;
uniform sampler2D srcTex;
uniform vec2 texelStep;
const float w0 = 0.227027;
const float w1 = 0.194595;
const float w2 = 0.121622;
const float w3 = 0.054054;
const float w4 = 0.016216;
void main()
{
    vec4 c = texture(srcTex, vUv) * w0;
    c += texture(srcTex, vUv + texelStep * 1.0) * w1;
    c += texture(srcTex, vUv - texelStep * 1.0) * w1;
    c += texture(srcTex, vUv + texelStep * 2.0) * w2;
    c += texture(srcTex, vUv - texelStep * 2.0) * w2;
    c += texture(srcTex, vUv + texelStep * 3.0) * w3;
    c += texture(srcTex, vUv - texelStep * 3.0) * w3;
    c += texture(srcTex, vUv + texelStep * 4.0) * w4;
    c += texture(srcTex, vUv - texelStep * 4.0) * w4;
    fragColor = c;
}
)";

constexpr const char* kCompositeFS = R"(
#version 150 core
in vec2 vUv;
out vec4 fragColor;
uniform sampler2D sceneTex;
uniform sampler2D bloomTex;
uniform float bloomStrength;
void main()
{
    vec3 scene = texture(sceneTex, vUv).rgb;
    vec3 bloom = texture(bloomTex, vUv).rgb;
    vec3 col = scene + bloom * bloomStrength;
    fragColor = vec4(col, 1.0);
}
)";

constexpr float kQuad[] = {
    -1.0f, -1.0f,
     3.0f, -1.0f,
    -1.0f,  3.0f,
};

void logGl (const juce::String& msg)
{
    juce::File f ("/tmp/manifold-gl.log");
    f.appendText (juce::Time::getCurrentTime().toString (true, true) + "  " + msg + "\n");
}

std::unique_ptr<juce::OpenGLShaderProgram> makeProgram (juce::OpenGLContext& ctx,
                                                        const char* vs,
                                                        const char* fs,
                                                        const juce::String& tag)
{
    auto p = std::make_unique<juce::OpenGLShaderProgram> (ctx);
    if (! p->addVertexShader (vs))
    {
        logGl ("vs compile failed [" + tag + "]: " + p->getLastError());
        return nullptr;
    }
    if (! p->addFragmentShader (fs))
    {
        logGl ("fs compile failed [" + tag + "]: " + p->getLastError());
        return nullptr;
    }
    if (! p->link())
    {
        logGl ("link failed [" + tag + "]: " + p->getLastError());
        return nullptr;
    }
    logGl ("shader ok [" + tag + "]");
    return p;
}

} // namespace

PhasePortrait::PhasePortrait (ManifoldProcessor& p) : processor (p)
{
    setOpaque (true);
    context.setOpenGLVersionRequired (juce::OpenGLContext::openGL3_2);
    context.setRenderer (this);
    context.setContinuousRepainting (true);
    context.attachTo (*this);
}

PhasePortrait::~PhasePortrait()
{
    context.detach();
}

void PhasePortrait::setBeaconsVisible (bool shouldBeVisible)
{
    beaconsVisible.store (shouldBeVisible);
    repaint();
}

void PhasePortrait::paint (juce::Graphics& g)
{
    if (! beaconsVisible.load()) return;

    constexpr float kPad = 0.92f;
    const auto bounds = getLocalBounds().toFloat();
    const float W = bounds.getWidth();
    const float H = bounds.getHeight();

    // Same NDC -> pixel mapping as the beacons. Y is flipped on the way to JUCE
    // coordinates (JUCE Y grows downward).
    auto ndcToScreen = [&] (float nx, float ny)
    {
        return juce::Point<float> { (nx * 0.5f + 0.5f) * W,
                                    (1.0f - (ny * 0.5f + 0.5f)) * H };
    };

    const auto pal = paletteFor (readFilterType (processor.getAPVTS()));
    g.setColour (pal.label);
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));

    constexpr int kInset = 6;
    const int labelW = 60, labelH = 14;

    // DRIVE — bottom edge, centered horizontally.
    {
        const auto p = ndcToScreen (0.0f, -kPad);
        const juce::Rectangle<int> r ((int) p.x - labelW / 2,
                                      (int) p.y - labelH - kInset,
                                      labelW, labelH);
        g.drawFittedText ("DRIVE", r, juce::Justification::centred, 1);
    }
    // CUTOFF — left edge, vertically centered.
    {
        const auto p = ndcToScreen (-kPad, 0.0f);
        const juce::Rectangle<int> r ((int) p.x + kInset,
                                      (int) p.y - labelH / 2,
                                      labelW, labelH);
        g.drawFittedText ("CUTOFF", r, juce::Justification::centredLeft, 1);
    }
    // RES — right edge, vertically centered.
    {
        const auto p = ndcToScreen (kPad, 0.0f);
        const juce::Rectangle<int> r ((int) p.x - labelW - kInset,
                                      (int) p.y - labelH / 2,
                                      labelW, labelH);
        g.drawFittedText ("RES", r, juce::Justification::centredRight, 1);
    }
    // MORPH — top edge, horizontally centered.
    {
        const auto p = ndcToScreen (0.0f, kPad);
        const juce::Rectangle<int> r ((int) p.x - labelW / 2,
                                      (int) p.y + kInset,
                                      labelW, labelH);
        g.drawFittedText ("MORPH", r, juce::Justification::centred, 1);
    }
}

void PhasePortrait::newOpenGLContextCreated()
{
    logGl (juce::String ("ctx created — GL_VERSION: ")
           + juce::String ((const char*) gl::glGetString (gl::GL_VERSION))
           + " | GLSL: "
           + juce::String ((const char*) gl::glGetString (gl::GL_SHADING_LANGUAGE_VERSION)));

    trailShader     = makeProgram (context, kTrailVS,  kTrailFS,    "trail");
    beaconShader    = makeProgram (context, kBeaconVS, kBeaconFS,   "beacon");
    blurShader      = makeProgram (context, kQuadVS,   kBlurFS,     "blur");
    compositeShader = makeProgram (context, kQuadVS,   kCompositeFS,"composite");

    gl::glGenVertexArrays (1, &trailVao);
    gl::glGenBuffers      (1, &trailVbo);
    gl::glBindVertexArray (trailVao);
    gl::glBindBuffer (gl::GL_ARRAY_BUFFER, trailVbo);
    gl::glBufferData (gl::GL_ARRAY_BUFFER, sizeof (Vertex) * kTrailSize * 2, nullptr, gl::GL_STREAM_DRAW);
    if (trailShader)
    {
        const auto posLoc = gl::glGetAttribLocation (trailShader->getProgramID(), "position");
        const auto ageLoc = gl::glGetAttribLocation (trailShader->getProgramID(), "age");
        const auto uLoc   = gl::glGetAttribLocation (trailShader->getProgramID(), "u");
        gl::glEnableVertexAttribArray ((unsigned int) posLoc);
        gl::glVertexAttribPointer ((unsigned int) posLoc, 2, gl::GL_FLOAT, gl::GL_FALSE,
                                   sizeof (Vertex), (void*) offsetof (Vertex, x));
        gl::glEnableVertexAttribArray ((unsigned int) ageLoc);
        gl::glVertexAttribPointer ((unsigned int) ageLoc, 1, gl::GL_FLOAT, gl::GL_FALSE,
                                   sizeof (Vertex), (void*) offsetof (Vertex, age));
        gl::glEnableVertexAttribArray ((unsigned int) uLoc);
        gl::glVertexAttribPointer ((unsigned int) uLoc, 1, gl::GL_FLOAT, gl::GL_FALSE,
                                   sizeof (Vertex), (void*) offsetof (Vertex, u));
    }

    gl::glGenVertexArrays (1, &beaconVao);
    gl::glGenBuffers      (1, &beaconVbo);
    gl::glBindVertexArray (beaconVao);
    gl::glBindBuffer (gl::GL_ARRAY_BUFFER, beaconVbo);
    gl::glBufferData (gl::GL_ARRAY_BUFFER, sizeof (Beacon) * kNumBeacons, nullptr, gl::GL_STREAM_DRAW);
    if (beaconShader)
    {
        const auto posLoc  = gl::glGetAttribLocation (beaconShader->getProgramID(), "position");
        const auto sizeLoc = gl::glGetAttribLocation (beaconShader->getProgramID(), "size");
        const auto intLoc  = gl::glGetAttribLocation (beaconShader->getProgramID(), "intensity");
        gl::glEnableVertexAttribArray ((unsigned int) posLoc);
        gl::glVertexAttribPointer ((unsigned int) posLoc, 2, gl::GL_FLOAT, gl::GL_FALSE,
                                   sizeof (Beacon), (void*) offsetof (Beacon, x));
        gl::glEnableVertexAttribArray ((unsigned int) sizeLoc);
        gl::glVertexAttribPointer ((unsigned int) sizeLoc, 1, gl::GL_FLOAT, gl::GL_FALSE,
                                   sizeof (Beacon), (void*) offsetof (Beacon, size));
        gl::glEnableVertexAttribArray ((unsigned int) intLoc);
        gl::glVertexAttribPointer ((unsigned int) intLoc, 1, gl::GL_FLOAT, gl::GL_FALSE,
                                   sizeof (Beacon), (void*) offsetof (Beacon, intensity));
    }

    gl::glGenVertexArrays (1, &quadVao);
    gl::glGenBuffers      (1, &quadVbo);
    gl::glBindVertexArray (quadVao);
    gl::glBindBuffer (gl::GL_ARRAY_BUFFER, quadVbo);
    gl::glBufferData (gl::GL_ARRAY_BUFFER, sizeof (kQuad), kQuad, gl::GL_STATIC_DRAW);
    // position attribute lives at the same name in both blur and composite VS.
    if (blurShader)
    {
        const auto posLoc = gl::glGetAttribLocation (blurShader->getProgramID(), "position");
        gl::glEnableVertexAttribArray ((unsigned int) posLoc);
        gl::glVertexAttribPointer ((unsigned int) posLoc, 2, gl::GL_FLOAT, gl::GL_FALSE,
                                   sizeof (float) * 2, nullptr);
    }

    gl::glBindVertexArray (0);
    gl::glBindBuffer (gl::GL_ARRAY_BUFFER, 0);
}

void PhasePortrait::openGLContextClosing()
{
    releaseGLResources();
}

void PhasePortrait::releaseGLResources()
{
    sceneFbo.release();
    blurHFbo.release();
    blurVFbo.release();
    if (trailVbo  != 0) { gl::glDeleteBuffers (1, &trailVbo);       trailVbo  = 0; }
    if (trailVao  != 0) { gl::glDeleteVertexArrays (1, &trailVao);  trailVao  = 0; }
    if (quadVbo   != 0) { gl::glDeleteBuffers (1, &quadVbo);        quadVbo   = 0; }
    if (quadVao   != 0) { gl::glDeleteVertexArrays (1, &quadVao);   quadVao   = 0; }
    if (beaconVbo != 0) { gl::glDeleteBuffers (1, &beaconVbo);      beaconVbo = 0; }
    if (beaconVao != 0) { gl::glDeleteVertexArrays (1, &beaconVao); beaconVao = 0; }
    trailShader.reset();
    beaconShader.reset();
    blurShader.reset();
    compositeShader.reset();
}

void PhasePortrait::ensureFbos (int w, int h)
{
    if (w == fboWidth && h == fboHeight && sceneFbo.isValid()) return;
    sceneFbo.release();
    blurHFbo.release();
    blurVFbo.release();
    sceneFbo.initialise (context, w, h);
    blurHFbo.initialise (context, w, h);
    blurVFbo.initialise (context, w, h);
    fboWidth = w; fboHeight = h;
}

void PhasePortrait::drainPipe()
{
    constexpr int kDrainMax = 512;
    std::array<ManifoldProcessor::PortraitPoint, kDrainMax> scratch {};
    const int got = processor.popPortraitPoints (scratch.data(), kDrainMax);
    for (int i = 0; i < got; ++i)
    {
        trail[(size_t) trailHead] = scratch[(size_t) i];
        trailHead = (trailHead + 1) % kTrailSize;
        if (trailCount < kTrailSize) ++trailCount;
    }
}

void PhasePortrait::writeTrailVbo()
{
    if (trailCount < 2 || fboWidth <= 0 || fboHeight <= 0) return;

    constexpr float kPad         = 0.92f;
    constexpr float kHalfWidthPx = 5.5f;

    const int oldest = trailCount < kTrailSize ? 0 : trailHead;
    const float W = (float) fboWidth;
    const float H = (float) fboHeight;

    // Project all trail points into pixel space — perpendiculars must be aspect-correct.
    for (int i = 0; i < trailCount; ++i)
    {
        const int idx = (oldest + i) % kTrailSize;
        const auto& p = trail[(size_t) idx];
        ptsScratch[(size_t) i] = {
            (p.x * kPad * 0.5f + 0.5f) * W,
            (p.z * kPad * 0.5f + 0.5f) * H,
        };
    }

    auto perpUnit = [] (juce::Point<float> v)
    {
        const float L = std::hypot (v.x, v.y);
        if (L < 1.0e-6f) return juce::Point<float> { 0.0f, 0.0f };
        return juce::Point<float> { -v.y / L, v.x / L };
    };

    auto toNdc = [&] (juce::Point<float> px)
    {
        return juce::Point<float> { px.x / W * 2.0f - 1.0f, px.y / H * 2.0f - 1.0f };
    };

    for (int i = 0; i < trailCount; ++i)
    {
        juce::Point<float> n;
        if (i == 0)
            n = perpUnit (ptsScratch[1] - ptsScratch[0]);
        else if (i == trailCount - 1)
            n = perpUnit (ptsScratch[(size_t) i] - ptsScratch[(size_t) (i - 1)]);
        else
        {
            const auto n1 = perpUnit (ptsScratch[(size_t) i]       - ptsScratch[(size_t) (i - 1)]);
            const auto n2 = perpUnit (ptsScratch[(size_t) (i + 1)] - ptsScratch[(size_t) i]);
            n = n1 + n2;
            const float L = std::hypot (n.x, n.y);
            if (L < 1.0e-6f)
                n = perpUnit (ptsScratch[(size_t) (i + 1)] - ptsScratch[(size_t) (i - 1)]);
            else
                n = { n.x / L, n.y / L };
        }

        const auto pPlus  = toNdc (ptsScratch[(size_t) i] + n * kHalfWidthPx);
        const auto pMinus = toNdc (ptsScratch[(size_t) i] - n * kHalfWidthPx);
        const float age = (float) i / (float) (trailCount - 1);

        vboScratch[(size_t) (i * 2 + 0)] = { pPlus .x, pPlus .y, age, +1.0f };
        vboScratch[(size_t) (i * 2 + 1)] = { pMinus.x, pMinus.y, age, -1.0f };
    }

    gl::glBindBuffer (gl::GL_ARRAY_BUFFER, trailVbo);
    gl::glBufferSubData (gl::GL_ARRAY_BUFFER, 0,
                         (GLsizeiptr) (sizeof (Vertex) * (size_t) (trailCount * 2)),
                         vboScratch.data());
    gl::glBindBuffer (gl::GL_ARRAY_BUFFER, 0);
}

void PhasePortrait::renderOpenGL()
{
    if (! trailShader || ! beaconShader || ! blurShader || ! compositeShader) return;

    const auto scale = (float) context.getRenderingScale();
    const int w = juce::jmax (1, (int) std::round (getWidth()  * scale));
    const int h = juce::jmax (1, (int) std::round (getHeight() * scale));
    ensureFbos (w, h);

    drainPipe();
    writeTrailVbo();

    // Beacons track the latest Lorenz state. Head marker sits at orbit head;
    // edge markers slide along the frame on the axis that drives each destination.
    const bool drawBeacons = beaconsVisible.load();
    if (drawBeacons && trailCount > 0)
    {
        constexpr float kPad = 0.92f;
        const int latest = (trailHead - 1 + kTrailSize) % kTrailSize;
        const auto& p = trail[(size_t) latest];

        // Point sizes are in device pixels; scale with DPI so retina still reads.
        const float head = 36.0f * scale;
        const float edge = 16.0f * scale;

        beaconBuf[0] = { p.x * kPad,  p.z * kPad,  head, 1.20f };      // head
        beaconBuf[1] = { p.x * kPad, -kPad,        edge, 0.95f };      // drive (bottom, x)
        beaconBuf[2] = { -kPad,       p.y * kPad,  edge, 0.95f };      // cutoff (left, y)
        beaconBuf[3] = {  kPad,       p.z * kPad,  edge, 0.95f };      // res (right, z)
        beaconBuf[4] = { p.z * kPad,  kPad,        edge, 0.95f };      // morph (top, z bipolar)

        gl::glBindBuffer (gl::GL_ARRAY_BUFFER, beaconVbo);
        gl::glBufferSubData (gl::GL_ARRAY_BUFFER, 0,
                             (GLsizeiptr) (sizeof (Beacon) * kNumBeacons),
                             beaconBuf.data());
        gl::glBindBuffer (gl::GL_ARRAY_BUFFER, 0);
    }

    // --- Pass 1: trail into sceneFbo, additive blending ---
    sceneFbo.makeCurrentRenderingTarget();
    gl::glViewport (0, 0, w, h);
    gl::glClearColor (0.02f, 0.02f, 0.04f, 1.0f);
    gl::glClear (gl::GL_COLOR_BUFFER_BIT);

    const auto pal = paletteFor (readFilterType (processor.getAPVTS()));

    if (trailCount >= 2)
    {
        gl::glEnable (gl::GL_BLEND);
        gl::glBlendFunc (gl::GL_ONE, gl::GL_ONE);  // additive

        trailShader->use();
        trailShader->setUniform ("trailColor", pal.trailR, pal.trailG, pal.trailB);
        gl::glBindVertexArray (trailVao);
        gl::glDrawArrays (gl::GL_TRIANGLE_STRIP, 0, trailCount * 2);
        gl::glBindVertexArray (0);

        if (drawBeacons)
        {
            // Beacons drawn into the same scene FBO so they feed the bloom pass.
            gl::glEnable (gl::GL_PROGRAM_POINT_SIZE);
            beaconShader->use();
            beaconShader->setUniform ("beaconColor", pal.beaconR, pal.beaconG, pal.beaconB);
            gl::glBindVertexArray (beaconVao);
            gl::glDrawArrays (gl::GL_POINTS, 0, kNumBeacons);
            gl::glBindVertexArray (0);
            gl::glDisable (gl::GL_PROGRAM_POINT_SIZE);
        }

        gl::glDisable (gl::GL_BLEND);
    }
    sceneFbo.releaseAsRenderingTarget();

    // --- Passes 2-N: ping-pong H+V blur. Each iteration widens the halo softly
    // without enlarging the 9-tap kernel (avoids ringing). Iteration `i` samples
    // at stride 2^i texels so the effective sigma roughly doubles each pass.
    constexpr int kBlurIterations = 4;
    auto* readFbo  = &sceneFbo;
    for (int i = 0; i < kBlurIterations; ++i)
    {
        const float stride = (float) (1 << i);

        blurHFbo.makeCurrentRenderingTarget();
        gl::glViewport (0, 0, w, h);
        gl::glClearColor (0, 0, 0, 1);
        gl::glClear (gl::GL_COLOR_BUFFER_BIT);
        blurShader->use();
        gl::glActiveTexture (gl::GL_TEXTURE0);
        gl::glBindTexture (gl::GL_TEXTURE_2D, readFbo->getTextureID());
        blurShader->setUniform ("srcTex", 0);
        blurShader->setUniform ("texelStep", stride / (float) w, 0.0f);
        gl::glBindVertexArray (quadVao);
        gl::glDrawArrays (gl::GL_TRIANGLES, 0, 3);
        gl::glBindVertexArray (0);
        blurHFbo.releaseAsRenderingTarget();

        blurVFbo.makeCurrentRenderingTarget();
        gl::glViewport (0, 0, w, h);
        gl::glClearColor (0, 0, 0, 1);
        gl::glClear (gl::GL_COLOR_BUFFER_BIT);
        blurShader->use();
        gl::glActiveTexture (gl::GL_TEXTURE0);
        gl::glBindTexture (gl::GL_TEXTURE_2D, blurHFbo.getTextureID());
        blurShader->setUniform ("srcTex", 0);
        blurShader->setUniform ("texelStep", 0.0f, stride / (float) h);
        gl::glBindVertexArray (quadVao);
        gl::glDrawArrays (gl::GL_TRIANGLES, 0, 3);
        gl::glBindVertexArray (0);
        blurVFbo.releaseAsRenderingTarget();

        readFbo = &blurVFbo;  // subsequent iterations widen the already-blurred result
    }

    // --- Pass 4: composite to screen ---
    gl::glViewport (0, 0, w, h);
    gl::glClearColor (0.02f, 0.02f, 0.04f, 1.0f);
    gl::glClear (gl::GL_COLOR_BUFFER_BIT);
    compositeShader->use();
    gl::glActiveTexture (gl::GL_TEXTURE0);
    gl::glBindTexture (gl::GL_TEXTURE_2D, sceneFbo.getTextureID());
    compositeShader->setUniform ("sceneTex", 0);
    gl::glActiveTexture (gl::GL_TEXTURE1);
    gl::glBindTexture (gl::GL_TEXTURE_2D, blurVFbo.getTextureID());
    compositeShader->setUniform ("bloomTex", 1);
    compositeShader->setUniform ("bloomStrength", 3.2f);
    gl::glBindVertexArray (quadVao);
    gl::glDrawArrays (gl::GL_TRIANGLES, 0, 3);
    gl::glBindVertexArray (0);
}

} // namespace manifold::ui
