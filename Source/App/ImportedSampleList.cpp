#include "ImportedSampleList.h"
#include "MiguelLookAndFeel.h"
#include "PrecisionRotarySlider.h"

namespace
{
class SampleSlotRow final : public juce::Component
{
public:
    std::function<void()> onMute;
    std::function<void(float)> onVolume;
    std::function<void(float)> onEq;

    SampleSlotRow()
    {
        setInterceptsMouseClicks(false, true);
        addAndMakeVisible(statusLed);
        addAndMakeVisible(name);
        addAndMakeVisible(volume);
        addAndMakeVisible(eq);
        name.setInterceptsMouseClicks(false, false);
        volume.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        volume.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        volume.setRange(0.0, 1.5, 0.01);
        volume.setValue(1.0, juce::dontSendNotification);
        volume.setWantsKeyboardFocus(false);
        volume.setMouseDragSensitivity(500);
        volume.setWheelStepMultiplier(5.0);
        eq.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        eq.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        eq.setRange(0.0, 1.0, 0.01);
        eq.setValue(0.5, juce::dontSendNotification);
        eq.setWantsKeyboardFocus(false);
        eq.setMouseDragSensitivity(500);
        eq.setWheelStepMultiplier(5.0);
        statusLed.onClick = [this]
        {
            if (onMute)
                onMute();
        };
        volume.onValueChange = [this]
        {
            if (onVolume)
                onVolume(static_cast<float>(volume.getValue()));
        };
        eq.onValueChange = [this]
        {
            if (onEq)
                onEq(static_cast<float>(eq.getValue()));
        };
    }

    void setContent(const juce::String& text, bool muted, float vol,
                    float eqAmt, bool selected)
    {
        chosen = selected;
        name.setText(text, juce::dontSendNotification);
        name.setColour(juce::Label::textColourId,
                       text.contains("—") ? MiguelColours::textMuted()
                                          : MiguelColours::text());
        statusLed.setLevel((selected && !muted) ? 1.0f : 0.0f, false);
        volume.setValue(vol, juce::dontSendNotification);
        eq.setValue(eqAmt, juce::dontSendNotification);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(chosen ? MiguelColours::green().withAlpha(0.28f)
                           : MiguelColours::panel());
        g.fillRect(bounds);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4, 2);
        statusLed.setBounds(bounds.removeFromLeft(22).reduced(1));
        eq.setBounds(bounds.removeFromRight(34).reduced(1));
        volume.setBounds(bounds.removeFromRight(34).reduced(1));
        name.setBounds(bounds.reduced(4, 0));
    }

private:
    bool chosen = false;
    SignalLed statusLed;
    juce::Label name;
    FineWheelSlider volume;
    FineWheelSlider eq;
};
}

ImportedSampleList::ImportedSampleList()
{
    addAndMakeVisible(list);
    list.setRowHeight(40);
    list.setMultipleSelectionEnabled(true);
    list.setClickingTogglesRowSelection(true);
    list.setColour(juce::ListBox::backgroundColourId,
                   MiguelColours::panel());
    list.setOutlineThickness(0);
    list.setWantsKeyboardFocus(false);
}

bool ImportedSampleList::isSupportedAudioFile(const juce::File& file)
{
    static const juce::StringArray extensions{
        ".wav", ".mp3", ".aif", ".aiff", ".flac", ".ogg"
    };
    return file.existsAsFile()
        && extensions.contains(file.getFileExtension().toLowerCase());
}

int ImportedSampleList::firstEmptySlot() const
{
    for (int i = 0; i < maxSamples; ++i)
        if (!slots[static_cast<size_t>(i)].file.existsAsFile())
            return i;
    return -1;
}

int ImportedSampleList::getNumSamples() const noexcept
{
    auto count = 0;
    for (const auto& slot : slots)
        if (slot.file.existsAsFile())
            ++count;
    return count;
}

void ImportedSampleList::addFiles(const juce::Array<juce::File>& files)
{
    auto firstAdded = -1;
    for (const auto& file : files)
    {
        if (!isSupportedAudioFile(file))
            continue;
        auto already = false;
        for (const auto& slot : slots)
            if (slot.file == file)
                already = true;
        if (already)
            continue;
        auto index = firstEmptySlot();
        if (index < 0)
            index = maxSamples - 1;
        slots[static_cast<size_t>(index)].file = file;
        slots[static_cast<size_t>(index)].muted = false;
        slots[static_cast<size_t>(index)].volume = 1.0f;
        slots[static_cast<size_t>(index)].eq = 0.5f;
        if (firstAdded < 0)
            firstAdded = index;
        if (onSlotLoaded)
            onSlotLoaded(index, file);
    }

    list.updateContent();
    if (firstAdded >= 0)
    {
        list.selectRow(firstAdded);
        list.scrollToEnsureRowIsOnscreen(firstAdded);
    }
    if (onSampleCountChanged)
        onSampleCountChanged(getNumSamples());
    repaint();
}

void ImportedSampleList::addFiles(const juce::StringArray& paths)
{
    juce::Array<juce::File> files;
    for (const auto& path : paths)
        files.add(juce::File(path));
    addFiles(files);
}

void ImportedSampleList::setFilesFromPaths(const juce::StringArray& paths)
{
    for (auto& slot : slots)
        slot = {};
    addFiles(paths);
}

juce::StringArray ImportedSampleList::getFilePaths() const
{
    juce::StringArray paths;
    for (const auto& slot : slots)
        if (slot.file.existsAsFile())
            paths.add(slot.file.getFullPathName());
    return paths;
}

void ImportedSampleList::addOrReplaceLast(const juce::File& file)
{
    if (!isSupportedAudioFile(file))
        return;
    auto index = firstEmptySlot();
    if (index < 0)
        index = maxSamples - 1;
    slots[static_cast<size_t>(index)].file = file;
    slots[static_cast<size_t>(index)].muted = false;
    slots[static_cast<size_t>(index)].volume = 1.0f;
    slots[static_cast<size_t>(index)].eq = 0.5f;
    list.updateContent();
    list.selectRow(index);
    list.scrollToEnsureRowIsOnscreen(index);
    if (onSlotLoaded)
        onSlotLoaded(index, file);
    if (onSampleCountChanged)
        onSampleCountChanged(getNumSamples());
    if (onSelectionChanged)
        onSelectionChanged(file);
    repaint();
}

void ImportedSampleList::removeSelected()
{
    const auto selected = list.getSelectedRows();
    juce::Array<int> rows;
    for (int i = 0; i < selected.size(); ++i)
        rows.add(selected[i]);

    if (rows.isEmpty())
        rows.add(list.getSelectedRow());

    rows.sort();
    for (int i = rows.size(); --i >= 0;)
    {
        const auto row = rows[i];
        if (!juce::isPositiveAndBelow(row, maxSamples))
            continue;
        slots[static_cast<size_t>(row)] = {};
        if (onSlotLoaded)
            onSlotLoaded(row, {});
    }

    list.deselectAllRows();
    list.updateContent();
    if (onSelectionChanged)
        onSelectionChanged(getSelectedFile());
    if (onSampleCountChanged)
        onSampleCountChanged(getNumSamples());
    repaint();
}

juce::File ImportedSampleList::getSelectedFile() const
{
    const auto row = list.getSelectedRow();
    return juce::isPositiveAndBelow(row, maxSamples)
        ? slots[static_cast<size_t>(row)].file : juce::File{};
}

juce::Array<juce::File> ImportedSampleList::getSelectedFiles() const
{
    juce::Array<juce::File> files;
    const auto selected = list.getSelectedRows();
    for (int i = 0; i < selected.size(); ++i)
    {
        const auto row = selected[i];
        if (juce::isPositiveAndBelow(row, maxSamples)
            && slots[static_cast<size_t>(row)].file.existsAsFile())
            files.add(slots[static_cast<size_t>(row)].file);
    }
    return files;
}

int ImportedSampleList::getSelectedSlot() const
{
    return list.getSelectedRow();
}

juce::Array<int> ImportedSampleList::getSelectedSlots() const
{
    juce::Array<int> rows;
    const auto selected = list.getSelectedRows();
    for (int i = 0; i < selected.size(); ++i)
    {
        const auto row = selected[i];
        if (juce::isPositiveAndBelow(row, maxSamples)
            && slots[static_cast<size_t>(row)].file.existsAsFile())
            rows.add(row);
    }
    if (rows.isEmpty())
    {
        const auto row = list.getSelectedRow();
        if (juce::isPositiveAndBelow(row, maxSamples)
            && slots[static_cast<size_t>(row)].file.existsAsFile())
            rows.add(row);
    }
    return rows;
}

juce::File ImportedSampleList::getSlotFile(int slot) const
{
    return juce::isPositiveAndBelow(slot, maxSamples)
        ? slots[static_cast<size_t>(slot)].file : juce::File{};
}

void ImportedSampleList::setSlotFile(int slot, const juce::File& file)
{
    if (!juce::isPositiveAndBelow(slot, maxSamples))
        return;
    if (file.existsAsFile() && isSupportedAudioFile(file))
        slots[static_cast<size_t>(slot)].file = file;
    else
        slots[static_cast<size_t>(slot)] = {};
    list.updateContent();
    if (onSlotLoaded)
        onSlotLoaded(slot, slots[static_cast<size_t>(slot)].file);
    if (onSampleCountChanged)
        onSampleCountChanged(getNumSamples());
}

void ImportedSampleList::setSlotMute(int slot, bool muted)
{
    if (!juce::isPositiveAndBelow(slot, maxSamples))
        return;
    slots[static_cast<size_t>(slot)].muted = muted;
    list.updateContent();
}

void ImportedSampleList::setSlotVolume(int slot, float volume)
{
    if (!juce::isPositiveAndBelow(slot, maxSamples))
        return;
    slots[static_cast<size_t>(slot)].volume = volume;
}

void ImportedSampleList::setSlotEq(int slot, float amount01)
{
    if (!juce::isPositiveAndBelow(slot, maxSamples))
        return;
    slots[static_cast<size_t>(slot)].eq = amount01;
}

bool ImportedSampleList::isSlotMuted(int slot) const
{
    return juce::isPositiveAndBelow(slot, maxSamples)
        && slots[static_cast<size_t>(slot)].muted;
}

float ImportedSampleList::getSlotVolume(int slot) const
{
    return juce::isPositiveAndBelow(slot, maxSamples)
        ? slots[static_cast<size_t>(slot)].volume : 1.0f;
}

float ImportedSampleList::getSlotEq(int slot) const
{
    return juce::isPositiveAndBelow(slot, maxSamples)
        ? slots[static_cast<size_t>(slot)].eq : 0.5f;
}

void ImportedSampleList::setSlotLed(int slot)
{
    if (!juce::isPositiveAndBelow(slot, maxSamples))
        return;
    if (auto* row = dynamic_cast<SampleSlotRow*>(
            list.getComponentForRowNumber(slot)))
    {
        const auto& data = slots[static_cast<size_t>(slot)];
        const auto title = data.file.existsAsFile()
            ? juce::String(slot + 1) + ". " + data.file.getFileName()
            : juce::String(slot + 1) + ". —";
        row->setContent(title, data.muted, data.volume, data.eq,
                        list.isRowSelected(slot));
    }
}

void ImportedSampleList::resized()
{
    list.setBounds(getLocalBounds().reduced(1));
}

void ImportedSampleList::paint(juce::Graphics& graphics)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    graphics.setColour(MiguelColours::panel());
    graphics.fillRoundedRectangle(bounds, 7.0f);
}

void ImportedSampleList::paintOverChildren(juce::Graphics& graphics)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    if (dragOver)
    {
        graphics.setColour(MiguelColours::orange().withAlpha(0.14f));
        graphics.fillRoundedRectangle(bounds, 7.0f);
    }
    graphics.setColour(dragOver ? MiguelColours::orange()
                                : MiguelColours::border());
    graphics.drawRoundedRectangle(bounds, 7.0f, dragOver ? 2.0f : 1.0f);
}

bool ImportedSampleList::isInterestedInFileDrag(
    const juce::StringArray& files)
{
    for (const auto& path : files)
        if (isSupportedAudioFile(juce::File(path)))
            return true;
    return false;
}

void ImportedSampleList::filesDropped(
    const juce::StringArray& files, int, int)
{
    dragOver = false;
    addFiles(files);
}

void ImportedSampleList::fileDragEnter(
    const juce::StringArray&, int, int)
{
    dragOver = true;
    repaint();
}

void ImportedSampleList::fileDragExit(const juce::StringArray&)
{
    dragOver = false;
    repaint();
}

int ImportedSampleList::getNumRows()
{
    return maxSamples;
}

void ImportedSampleList::paintListBoxItem(
    int, juce::Graphics&, int, int, bool)
{
}

juce::Component* ImportedSampleList::refreshComponentForRow(
    int row, bool selected, juce::Component* existing)
{
    auto* rowComp = existing != nullptr
        ? dynamic_cast<SampleSlotRow*>(existing)
        : new SampleSlotRow();
    if (rowComp == nullptr)
        rowComp = new SampleSlotRow();
    if (!juce::isPositiveAndBelow(row, maxSamples))
        return rowComp;

    const auto& slot = slots[static_cast<size_t>(row)];
    const auto title = slot.file.existsAsFile()
        ? juce::String(row + 1) + ". " + slot.file.getFileName()
        : juce::String(row + 1) + ". —";
    rowComp->setContent(title, slot.muted, slot.volume, slot.eq, selected);
    rowComp->onMute = [this, row]
    {
        slots[static_cast<size_t>(row)].muted =
            !slots[static_cast<size_t>(row)].muted;
        if (onMuteChanged)
            onMuteChanged(row, slots[static_cast<size_t>(row)].muted);
        list.updateContent();
    };
    rowComp->onVolume = [this, row](float value)
    {
        slots[static_cast<size_t>(row)].volume = value;
        if (onVolumeChanged)
            onVolumeChanged(row, value);
    };
    rowComp->onEq = [this, row](float value)
    {
        slots[static_cast<size_t>(row)].eq = value;
        if (onEqChanged)
            onEqChanged(row, value);
    };
    rowComp->setOpaque(true);
    return rowComp;
}

void ImportedSampleList::selectedRowsChanged(int row)
{
    if (onSelectionChanged && juce::isPositiveAndBelow(row, maxSamples))
        onSelectionChanged(slots[static_cast<size_t>(row)].file);
}

void ImportedSampleList::listBoxItemDoubleClicked(
    int row, const juce::MouseEvent&)
{
    if (onFileDoubleClicked && juce::isPositiveAndBelow(row, maxSamples)
        && slots[static_cast<size_t>(row)].file.existsAsFile())
        onFileDoubleClicked(slots[static_cast<size_t>(row)].file);
}

void ImportedSampleList::removeRow(int row)
{
    if (!juce::isPositiveAndBelow(row, maxSamples))
        return;

    slots[static_cast<size_t>(row)] = {};
    list.updateContent();
    if (onSlotLoaded)
        onSlotLoaded(row, {});
    if (onSelectionChanged)
        onSelectionChanged(getSelectedFile());
    if (onSampleCountChanged)
        onSampleCountChanged(getNumSamples());
    repaint();
}
