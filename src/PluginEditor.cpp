#include "PluginEditor.h"

static constexpr int kW = 620;
static constexpr int kH = 420;

// Palette
static const juce::Colour kBg      { 0xff0d0d0d };
static const juce::Colour kPanel   { 0xff1a1a1a };
static const juce::Colour kAccent  { 0xffb52a2a }; // arterial red
static const juce::Colour kText    { 0xffdddddd };
static const juce::Colour kSubText { 0xff666666 };

DissolutionEditor::DissolutionEditor(DissolutionProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setSize(kW, kH);

    auto setupAll = [&](juce::Slider& s, juce::Label& l, const juce::String& name) {
        setupSlider(s, l, name);
    };

    setupAll(fuzzDrive,  fuzzDriveLbl,   "DRIVE");
    setupAll(fuzzTone,   fuzzToneLbl,    "TONE");
    setupAll(fuzzOctave, fuzzOctaveLbl,  "OCT+");
    setupAll(fbChaos,    fbChaosLbl,     "CHAOS");
    setupAll(fbFreq,     fbFreqLbl,      "FREQ");
    setupAll(crushBits,  crushBitsLbl,   "BITS");
    setupAll(crushRate,  crushRateLbl,   "RATE");
    setupAll(revSize,    revSizeLbl,     "SIZE");
    setupAll(revDamp,    revDampLbl,     "DAMP");
    setupAll(revShimmer, revShimmerLbl,  "SHIMMER");
    setupAll(revMix,     revMixLbl,      "MIX");
    setupAll(noise,      noiseLbl,       "NOISE");
    setupAll(wetDry,     wetDryLbl,      "WET/DRY");

    freezeBtn.setButtonText("FREEZE");
    freezeBtn.setClickingTogglesState(true);
    freezeBtn.setColour(juce::TextButton::buttonColourId,    kPanel);
    freezeBtn.setColour(juce::TextButton::buttonOnColourId,  kAccent);
    freezeBtn.setColour(juce::TextButton::textColourOffId,   kSubText);
    freezeBtn.setColour(juce::TextButton::textColourOnId,    kText);
    addAndMakeVisible(freezeBtn);

    attachAll();
}

void DissolutionEditor::setupSlider(juce::Slider& s, juce::Label& l, const juce::String& name)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setColour(juce::Slider::rotarySliderFillColourId,   kAccent);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, kSubText);
    s.setColour(juce::Slider::thumbColourId,               kText);
    addAndMakeVisible(s);

    l.setText(name, juce::dontSendNotification);
    l.setFont(juce::Font(10.0f, juce::Font::plain));
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, kSubText);
    addAndMakeVisible(l);
}

void DissolutionEditor::attachAll()
{
    auto& vts = proc.apvts;
    fuzzDriveA  = std::make_unique<Attach>(vts, "fuzz_drive",  fuzzDrive);
    fuzzToneA   = std::make_unique<Attach>(vts, "fuzz_tone",   fuzzTone);
    fuzzOctaveA = std::make_unique<Attach>(vts, "fuzz_octave", fuzzOctave);
    fbChaosA    = std::make_unique<Attach>(vts, "fb_chaos",    fbChaos);
    fbFreqA     = std::make_unique<Attach>(vts, "fb_freq",     fbFreq);
    crushBitsA  = std::make_unique<Attach>(vts, "crush_bits",  crushBits);
    crushRateA  = std::make_unique<Attach>(vts, "crush_rate",  crushRate);
    revSizeA    = std::make_unique<Attach>(vts, "rev_size",    revSize);
    revDampA    = std::make_unique<Attach>(vts, "rev_damp",    revDamp);
    revShimmerA = std::make_unique<Attach>(vts, "rev_shimmer", revShimmer);
    revMixA     = std::make_unique<Attach>(vts, "rev_mix",     revMix);
    noiseA      = std::make_unique<Attach>(vts, "noise",       noise);
    wetDryA     = std::make_unique<Attach>(vts, "wet_dry",     wetDry);
    freezeA     = std::make_unique<BAttach>(vts, "freeze",     freezeBtn);
}

void DissolutionEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBg);

    // Title
    g.setColour(kAccent);
    g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawText("DISSOLUTION", 0, 8, kW, 28, juce::Justification::centred);

    g.setColour(kSubText);
    g.setFont(juce::Font(9.0f));
    g.drawText("noise / shimmer / obliteration", 0, 32, kW, 14, juce::Justification::centred);

    // Section labels
    auto drawSectionLabel = [&](const juce::String& t, int x, int y, int w) {
        g.setColour(kAccent.withAlpha(0.5f));
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText(t, x, y, w, 14, juce::Justification::left);
        g.drawLine(static_cast<float>(x), static_cast<float>(y + 12),
                   static_cast<float>(x + w), static_cast<float>(y + 12), 0.5f);
    };

    drawSectionLabel("FUZZ",     14,  54, 175);
    drawSectionLabel("CHAOS",   200,  54, 130);
    drawSectionLabel("CRUSH",   340,  54, 130);
    drawSectionLabel("SHIMMER REVERB", 14, 230, 410);
    drawSectionLabel("GLOBAL",  440, 230, 165);
}

void DissolutionEditor::resized()
{
    auto knob = [](juce::Slider& s, juce::Label& l, int x, int y) {
        s.setBounds(x, y, 52, 52);
        l.setBounds(x, y + 52, 52, 14);
    };

    // Row 1: FUZZ (3 knobs) | CHAOS (2 knobs) | CRUSH (2 knobs)
    knob(fuzzDrive,  fuzzDriveLbl,   18,  72);
    knob(fuzzTone,   fuzzToneLbl,    76,  72);
    knob(fuzzOctave, fuzzOctaveLbl, 134,  72);

    knob(fbChaos, fbChaosLbl, 204,  72);
    knob(fbFreq,  fbFreqLbl,  262,  72);

    knob(crushBits, crushBitsLbl, 344,  72);
    knob(crushRate, crushRateLbl, 402,  72);

    // Row 2: SHIMMER REVERB (4 knobs) + FREEZE button
    knob(revSize,    revSizeLbl,    18, 248);
    knob(revDamp,    revDampLbl,    76, 248);
    knob(revShimmer, revShimmerLbl,134, 248);
    knob(revMix,     revMixLbl,    192, 248);

    // GLOBAL: noise + wet/dry
    knob(noise,  noiseLbl,  444, 248);
    knob(wetDry, wetDryLbl, 502, 248);

    // FREEZE button (large toggle)
    freezeBtn.setBounds(300, 254, 120, 40);
}
