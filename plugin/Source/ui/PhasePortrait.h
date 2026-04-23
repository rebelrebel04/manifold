#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include "../PluginProcessor.h"

namespace manifold::ui
{

// GL-rendered (x, z) projection of the Lorenz state.
// Trail accumulates with additive blending, then a two-pass gaussian blur creates
// the bloom halo before compositing back onto a black scene.
class PhasePortrait : public juce::Component, public juce::OpenGLRenderer
{
public:
    explicit PhasePortrait (ManifoldProcessor& processor);
    ~PhasePortrait() override;

    void setBeaconsVisible (bool shouldBeVisible);

    void paint (juce::Graphics&) override;  // labels overlay (composited above GL)

    // OpenGLRenderer
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

private:
    static constexpr int kTrailSize = 1500;

    // Two ribbon vertices per trail point (one offset to each side of the line).
    struct Vertex  { float x, y, age, u; };
    struct Beacon  { float x, y, size, intensity; };
    static constexpr int kNumBeacons = 5;  // 1 head + 4 edge (drive/cutoff/res/morph)

    void drainPipe();
    void writeTrailVbo();
    void ensureFbos (int w, int h);
    void releaseGLResources();

    ManifoldProcessor& processor;
    juce::OpenGLContext context;

    std::array<ManifoldProcessor::PortraitPoint, kTrailSize> trail {};
    int trailHead  = 0;
    int trailCount = 0;
    std::atomic<bool> beaconsVisible { false };

    std::unique_ptr<juce::OpenGLShaderProgram> trailShader, beaconShader, blurShader, compositeShader;
    juce::OpenGLFrameBuffer sceneFbo, blurHFbo, blurVFbo;
    int fboWidth = 0, fboHeight = 0;

    unsigned int trailVao  = 0, trailVbo  = 0;
    unsigned int quadVao   = 0, quadVbo   = 0;
    unsigned int beaconVao = 0, beaconVbo = 0;

    std::array<Vertex, kTrailSize * 2>         vboScratch  {};
    std::array<juce::Point<float>, kTrailSize> ptsScratch  {};
    std::array<Beacon, kNumBeacons>            beaconBuf   {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhasePortrait)
};

} // namespace manifold::ui
