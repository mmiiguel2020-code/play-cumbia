#include "ImportedSampleList.h"
#include "MiguelLookAndFeel.h"

ImportedSampleList::ImportedSampleList()
{
    addAndMakeVisible(list);
    list.setRowHeight(34);
    list.setMultipleSelectionEnabled(false);
    list.setColour(juce::ListBox::backgroundColourId,
                   MiguelColours::panel());
    list.setOutlineThickness(0);
}

bool ImportedSampleList::isSupportedAudioFile(const juce::File& file)
{
    static const juce::StringArray extensions{
        ".wav", ".mp3", ".aif", ".aiff", ".flac", ".ogg"
    };
    return file.existsAsFile()
        && extensions.contains(file.getFileExtension().toLowerCase());
}

void ImportedSampleList::addFiles(const juce::Array<juce::File>& files)
{
    auto firstAdded = -1;
    for (const auto& file : files)
    {
        if (!isSupportedAudioFile(file) || samples.contains(file))
            continue;
        if (firstAdded < 0)
            firstAdded = samples.size();
        samples.add(file);
    }

    list.updateContent();
    if (firstAdded >= 0)
    {
        list.selectRow(firstAdded);
        list.scrollToEnsureRowIsOnscreen(firstAdded);
    }
    if (onSampleCountChanged)
        onSampleCountChanged(samples.size());
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
    samples.clear();
    addFiles(paths);
}

juce::StringArray ImportedSampleList::getFilePaths() const
{
    juce::StringArray paths;
    for (const auto& file : samples)
        paths.add(file.getFullPathName());
    return paths;
}

void ImportedSampleList::removeSelected()
{
    removeRow(list.getSelectedRow());
}

juce::File ImportedSampleList::getSelectedFile() const
{
    const auto row = list.getSelectedRow();
    return juce::isPositiveAndBelow(row, samples.size())
        ? samples.getReference(row) : juce::File{};
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

    if (samples.isEmpty())
    {
        graphics.setColour(MiguelColours::textMuted());
        graphics.setFont(juce::FontOptions(14.0f));
        graphics.drawFittedText(
            "Arrastra archivos de audio aquí o usa \"Agregar samples...\"",
            getLocalBounds().reduced(24), juce::Justification::centred, 2);
    }
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
    return samples.size();
}

void ImportedSampleList::paintListBoxItem(
    int row, juce::Graphics& graphics, int width, int height, bool selected)
{
    if (!juce::isPositiveAndBelow(row, samples.size()))
        return;

    if (selected)
    {
        graphics.setColour(MiguelColours::orange().withAlpha(0.22f));
        graphics.fillRoundedRectangle(
            juce::Rectangle<float>(2.0f, 2.0f,
                static_cast<float>(width - 4),
                static_cast<float>(height - 4)), 4.0f);
    }
    else if (row % 2 != 0)
    {
        graphics.fillAll(MiguelColours::panelRaised().withAlpha(0.45f));
    }

    const auto& file = samples.getReference(row);
    graphics.setColour(selected ? MiguelColours::text()
                                : MiguelColours::textMuted());
    graphics.setFont(juce::FontOptions(13.0f));
    graphics.drawText(file.getFileName(), 12, 0, width - 140, height,
                      juce::Justification::centredLeft, true);

    const auto sizeMb = static_cast<double>(file.getSize())
        / (1024.0 * 1024.0);
    graphics.setColour(MiguelColours::textMuted().withAlpha(0.75f));
    graphics.setFont(juce::FontOptions(11.0f));
    graphics.drawText(juce::String(sizeMb, 1) + " MB",
                      width - 128, 0, 76, height,
                      juce::Justification::centredRight);

    const auto removeArea = juce::Rectangle<float>(
        static_cast<float>(width - 42), 7.0f, 24.0f,
        static_cast<float>(height - 14));
    graphics.setColour(MiguelColours::danger().withAlpha(
        selected ? 0.9f : 0.55f));
    graphics.drawLine(removeArea.getX() + 6.0f,
                      removeArea.getY() + 3.0f,
                      removeArea.getRight() - 6.0f,
                      removeArea.getBottom() - 3.0f, 1.8f);
    graphics.drawLine(removeArea.getRight() - 6.0f,
                      removeArea.getY() + 3.0f,
                      removeArea.getX() + 6.0f,
                      removeArea.getBottom() - 3.0f, 1.8f);
}

void ImportedSampleList::selectedRowsChanged(int row)
{
    if (onSelectionChanged && juce::isPositiveAndBelow(row, samples.size()))
        onSelectionChanged(samples.getReference(row));
}

void ImportedSampleList::listBoxItemClicked(
    int row, const juce::MouseEvent& event)
{
    if (event.position.x >= static_cast<float>(list.getWidth() - 48))
        removeRow(row);
}

void ImportedSampleList::listBoxItemDoubleClicked(
    int row, const juce::MouseEvent&)
{
    if (onFileDoubleClicked && juce::isPositiveAndBelow(row, samples.size()))
        onFileDoubleClicked(samples.getReference(row));
}

void ImportedSampleList::removeRow(int row)
{
    if (!juce::isPositiveAndBelow(row, samples.size()))
        return;

    samples.remove(row);
    list.updateContent();
    if (!samples.isEmpty())
        list.selectRow(juce::jmin(row, samples.size() - 1));
    else if (onSelectionChanged)
        onSelectionChanged({});
    if (onSampleCountChanged)
        onSampleCountChanged(samples.size());
    repaint();
}
