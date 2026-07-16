#include "MixAnalyzerComponent.h"
#include "MiguelLookAndFeel.h"

MixAnalyzerComponent::MixAnalyzerComponent()
{
    setOpaque(false);
}

void MixAnalyzerComponent::setLevels(
    float leftRmsDb, float rightRmsDb, float leftPeakDb,
    float rightPeakDb, float correlation)
{
    leftRms = leftRmsDb;
    rightRms = rightRmsDb;
    leftPeak = juce::jmax(leftPeakDb, leftPeak - 0.8f);
    rightPeak = juce::jmax(rightPeakDb, rightPeak - 0.8f);
    stereoCorrelation = correlation;
    repaint();
}

void MixAnalyzerComponent::pushAudio(const float* samples, int count)
{
    if (samples == nullptr || count <= 0)
        return;
    for (int index = 0; index < count; ++index)
    {
        const auto sample = samples[index];
        waveform[static_cast<size_t>(waveformWritePosition)] = sample;
        waveformWritePosition = (waveformWritePosition + 1) % waveformSize;
        fftData[static_cast<size_t>(fftWritePosition++)] = sample;
        if (fftWritePosition == fftSize)
        {
            calculateSpectrum();
            fftWritePosition = 0;
        }
    }
    repaint();
}

void MixAnalyzerComponent::calculateSpectrum()
{
    std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data());
    for (int bin = 0; bin < fftSize / 2; ++bin)
    {
        const auto db = juce::Decibels::gainToDecibels(
            fftData[static_cast<size_t>(bin)] / fftSize, -100.0f);
        const auto target = juce::jlimit(
            0.0f, 1.0f, (db + 90.0f) / 90.0f);
        spectrum[static_cast<size_t>(bin)] =
            juce::jmax(target,
                       spectrum[static_cast<size_t>(bin)] * 0.82f);
    }
}

float MixAnalyzerComponent::dbToProportion(float db)
{
    return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
}

void MixAnalyzerComponent::paint(juce::Graphics& graphics)
{
    const auto outer = getLocalBounds().toFloat().reduced(1.0f);
    juce::ColourGradient background(
        MiguelColours::panelRaised(), outer.getCentreX(), outer.getY(),
        MiguelColours::panel(), outer.getCentreX(), outer.getBottom(), false);
    graphics.setGradientFill(background);
    graphics.fillRoundedRectangle(outer, 8.0f);
    graphics.setColour(MiguelColours::border());
    graphics.drawRoundedRectangle(outer, 8.0f, 1.0f);

    auto content = outer.reduced(14.0f);
    auto metersArea = content.removeFromLeft(106.0f);
    content.removeFromLeft(12.0f);
    auto waveformArea = content.removeFromTop(content.getHeight() * 0.42f);
    content.removeFromTop(10.0f);
    auto spectrumArea = content;

    graphics.setColour(MiguelColours::textMuted());
    graphics.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    graphics.drawText("L", juce::Rectangle<float>(
                          metersArea.getX() + 12.0f, metersArea.getY(),
                          24.0f, 16.0f),
                      juce::Justification::centred);
    graphics.drawText("R", juce::Rectangle<float>(
                          metersArea.getX() + 52.0f, metersArea.getY(),
                          24.0f, 16.0f),
                      juce::Justification::centred);

    const auto meterTop = metersArea.getY() + 20.0f;
    const auto meterBottom = metersArea.getBottom() - 42.0f;
    const auto meterHeight = meterBottom - meterTop;
    const auto drawMeter = [&](float x, float rms, float peak)
    {
        auto track = juce::Rectangle<float>(x, meterTop, 24.0f, meterHeight);
        graphics.setColour(MiguelColours::background());
        graphics.fillRoundedRectangle(track, 3.0f);
        const auto amount = dbToProportion(rms);
        auto fill = track.withTrimmedTop(track.getHeight() * (1.0f - amount));
        juce::ColourGradient meterGradient(
            MiguelColours::danger(), fill.getCentreX(), track.getY(),
            MiguelColours::green(), fill.getCentreX(), track.getBottom(),
            false);
        meterGradient.addColour(0.36, MiguelColours::yellow());
        graphics.setGradientFill(meterGradient);
        graphics.fillRoundedRectangle(fill, 3.0f);
        const auto peakY = track.getBottom()
            - dbToProportion(peak) * track.getHeight();
        graphics.setColour(peak > -0.3f ? MiguelColours::danger()
                                        : MiguelColours::text());
        graphics.fillRect(track.getX(), peakY, track.getWidth(), 2.0f);
    };
    drawMeter(metersArea.getX() + 12.0f, leftRms, leftPeak);
    drawMeter(metersArea.getX() + 52.0f, rightRms, rightPeak);

    graphics.setColour(MiguelColours::textMuted());
    graphics.setFont(juce::FontOptions(10.0f));
    graphics.drawText("CORR", juce::Rectangle<float>(
                          metersArea.getX(), meterBottom + 8.0f,
                          88.0f, 14.0f),
                      juce::Justification::centred);
    auto correlationTrack = juce::Rectangle<float>(
        metersArea.getX() + 8.0f, meterBottom + 24.0f, 72.0f, 7.0f);
    graphics.setColour(MiguelColours::background());
    graphics.fillRoundedRectangle(correlationTrack, 3.0f);
    const auto correlationX = juce::jmap(
        stereoCorrelation, -1.0f, 1.0f,
        correlationTrack.getX(), correlationTrack.getRight());
    graphics.setColour(stereoCorrelation < 0.0f
                           ? MiguelColours::danger()
                           : MiguelColours::cyan());
    graphics.fillEllipse(correlationX - 4.0f,
                         correlationTrack.getCentreY() - 4.0f,
                         8.0f, 8.0f);

    const auto drawPanel = [&](juce::Rectangle<float> area,
                               const juce::String& title)
    {
        graphics.setColour(MiguelColours::background().withAlpha(0.68f));
        graphics.fillRoundedRectangle(area, 5.0f);
        graphics.setColour(MiguelColours::textMuted());
        graphics.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        graphics.drawText(title, area.reduced(8.0f).removeFromTop(14.0f),
                          juce::Justification::centredLeft);
    };
    drawPanel(waveformArea, "WAVEFORM");
    drawPanel(spectrumArea, "SPECTRUM");

    auto waveGraph = waveformArea.reduced(8.0f).withTrimmedTop(16.0f);
    const auto centreY = waveGraph.getCentreY();
    graphics.setColour(MiguelColours::border().withAlpha(0.6f));
    graphics.drawHorizontalLine(juce::roundToInt(centreY),
                                waveGraph.getX(), waveGraph.getRight());
    juce::Path wavePath;
    for (int index = 0; index < waveformSize; ++index)
    {
        const auto source = (waveformWritePosition + index) % waveformSize;
        const auto x = juce::jmap(static_cast<float>(index),
            0.0f, static_cast<float>(waveformSize - 1),
            waveGraph.getX(), waveGraph.getRight());
        const auto y = centreY - waveform[static_cast<size_t>(source)]
            * waveGraph.getHeight() * 0.45f;
        if (index == 0)
            wavePath.startNewSubPath(x, y);
        else
            wavePath.lineTo(x, y);
    }
    graphics.setColour(MiguelColours::cyan());
    graphics.strokePath(wavePath, juce::PathStrokeType(1.4f));

    auto spectrumGraph = spectrumArea.reduced(8.0f).withTrimmedTop(16.0f);
    juce::Path spectrumPath;
    spectrumPath.startNewSubPath(
        spectrumGraph.getX(), spectrumGraph.getBottom());
    for (int xIndex = 0; xIndex < juce::roundToInt(spectrumGraph.getWidth());
         ++xIndex)
    {
        const auto proportion = xIndex
            / juce::jmax(1.0f, spectrumGraph.getWidth() - 1.0f);
        const auto frequency = 20.0
            * std::pow(1000.0, static_cast<double>(proportion));
        const auto bin = juce::jlimit(
            0, fftSize / 2 - 1,
            static_cast<int>(frequency / 22050.0 * (fftSize / 2)));
        const auto x = spectrumGraph.getX() + xIndex;
        const auto y = spectrumGraph.getBottom()
            - spectrum[static_cast<size_t>(bin)] * spectrumGraph.getHeight();
        spectrumPath.lineTo(x, y);
    }
    spectrumPath.lineTo(spectrumGraph.getRight(), spectrumGraph.getBottom());
    spectrumPath.closeSubPath();
    juce::ColourGradient spectrumFill(
        MiguelColours::purple().withAlpha(0.6f),
        spectrumGraph.getCentreX(), spectrumGraph.getY(),
        MiguelColours::cyan().withAlpha(0.05f),
        spectrumGraph.getCentreX(), spectrumGraph.getBottom(), false);
    graphics.setGradientFill(spectrumFill);
    graphics.fillPath(spectrumPath);
    graphics.setColour(MiguelColours::purple());
    graphics.strokePath(spectrumPath, juce::PathStrokeType(1.2f));
}
