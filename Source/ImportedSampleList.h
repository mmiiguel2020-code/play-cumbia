#pragma once

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
    static constexpr int maxSamples = 32;
    juce::File getSelectedFile() const;
    int getNumSamples() const noexcept { return samples.size(); }

    std::function<void(const juce::File&)> onSelectionChanged;
    std::function<void(const juce::File&)> onFileDoubleClicked;
    std::function<void(int)> onSampleCountChanged;

    void resized() override;
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int, int) override;
    void fileDragEnter(const juce::StringArray&, int, int) override;
    void fileDragExit(const juce::StringArray&) override;

private:
    int getNumRows() override;
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override;
    void selectedRowsChanged(int) override;
    void listBoxItemClicked(int, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked(int, const juce::MouseEvent&) override;
    static bool isSupportedAudioFile(const juce::File&);
    void removeRow(int);

    juce::Array<juce::File> samples;
    juce::ListBox list{ "Samples importados", this };
    bool dragOver = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImportedSampleList)
};
