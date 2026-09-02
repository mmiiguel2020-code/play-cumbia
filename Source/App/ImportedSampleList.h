#pragma once

#include "RackPanel.h"

#include <array>
#include <functional>
#include <juce_gui_extra/juce_gui_extra.h>

class ImportedSampleList final : public juce::Component,
                                 private juce::ListBoxModel,
                                 public juce::FileDragAndDropTarget
{
public:
    ImportedSampleList();

    void addFiles(const juce::Array<juce::File>& files);
    void addFiles(const juce::StringArray& paths);
    void setFilesFromPaths(const juce::StringArray& paths);
    juce::StringArray getFilePaths() const;
    void addOrReplaceLast(const juce::File& file);
    void removeSelected();
    static constexpr int maxSamples = 16;
    juce::Array<int> getSelectedSlots() const;
    juce::File getSelectedFile() const;
    juce::Array<juce::File> getSelectedFiles() const;
    int getNumSamples() const noexcept;
    int getSelectedSlot() const;
    juce::File getSlotFile(int slot) const;
    void setSlotFile(int slot, const juce::File& file);
    void setSlotMute(int slot, bool muted);
    void setSlotVolume(int slot, float volume);
    void setSlotEq(int slot, float amount01);
    bool isSlotMuted(int slot) const;
    float getSlotVolume(int slot) const;
    float getSlotEq(int slot) const;
    void setSlotLed(int slot);

    std::function<void(const juce::File&)> onSelectionChanged;
    std::function<void(const juce::File&)> onFileDoubleClicked;
    std::function<void(int)> onSampleCountChanged;
    std::function<void(int, bool)> onMuteChanged;
    std::function<void(int, float)> onVolumeChanged;
    std::function<void(int, float)> onEqChanged;
    std::function<void(int, const juce::File&)> onSlotLoaded;

    void resized() override;
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int, int) override;
    void fileDragEnter(const juce::StringArray&, int, int) override;
    void fileDragExit(const juce::StringArray&) override;
    juce::Component* refreshComponentForRow(int, bool,
                                            juce::Component*) override;

private:
    int getNumRows() override;
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override;
    void selectedRowsChanged(int) override;
    void listBoxItemDoubleClicked(int, const juce::MouseEvent&) override;
    static bool isSupportedAudioFile(const juce::File&);
    void removeRow(int);
    int firstEmptySlot() const;

    struct SlotData
    {
        juce::File file;
        bool muted = false;
        float volume = 1.0f;
        float eq = 0.5f;
        float led = 0.0f;
    };

    std::array<SlotData, maxSamples> slots{};
    juce::ListBox list{ "Samples importados", this };
    bool dragOver = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImportedSampleList)
};
