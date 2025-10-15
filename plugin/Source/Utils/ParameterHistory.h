#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <vector>

namespace auralis::utils
{
class ParameterHistory : private juce::Timer, private juce::ValueTree::Listener
{
public:
    explicit ParameterHistory(juce::AudioProcessorValueTreeState& stateToWatch)
        : parameters(stateToWatch)
    {
        parameters.state.addListener(this);
        pushSnapshotFromCurrentState();
    }

    ~ParameterHistory() override
    {
        stopTimer();
        parameters.state.removeListener(this);
    }

    void captureSnapshotSoon()
    {
        if (isRestoring)
            return;

        pendingSnapshot = true;
        startTimer(snapshotDelayMs);
    }

    void captureSnapshotNow()
    {
        if (isRestoring)
            return;

        stopTimer();
        pendingSnapshot = false;
        pushSnapshotFromCurrentState();
    }

    bool undo()
    {
        if (! canUndo())
            return false;

        const juce::ScopedValueSetter<bool> restoringSetter(isRestoring, true);
        --currentIndex;
        restoreCurrentSnapshot();
        return true;
    }

    bool redo()
    {
        if (! canRedo())
            return false;

        const juce::ScopedValueSetter<bool> restoringSetter(isRestoring, true);
        ++currentIndex;
        restoreCurrentSnapshot();
        return true;
    }

    [[nodiscard]] bool canUndo() const noexcept
    {
        return currentIndex > 0 && currentIndex <= static_cast<int>(snapshots.size()) - 1;
    }

    [[nodiscard]] bool canRedo() const noexcept
    {
        return currentIndex >= 0 && currentIndex < static_cast<int>(snapshots.size()) - 1;
    }

private:
    void timerCallback() override
    {
        stopTimer();

        if (! pendingSnapshot || isRestoring)
            return;

        pendingSnapshot = false;
        pushSnapshotFromCurrentState();
    }

    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override
    {
        captureSnapshotSoon();
    }

    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}
    void valueTreeRedirected(juce::ValueTree&) override {}

    void pushSnapshotFromCurrentState()
    {
        auto snapshot = parameters.copyState().createCopy();

        if (currentIndex >= 0 && currentIndex < static_cast<int>(snapshots.size()) - 1)
            snapshots.erase(snapshots.begin() + currentIndex + 1, snapshots.end());

        snapshots.push_back(std::move(snapshot));

        if (snapshots.size() > maxSnapshots)
        {
            snapshots.erase(snapshots.begin());
        }

        currentIndex = static_cast<int>(snapshots.size()) - 1;
    }

    void restoreCurrentSnapshot()
    {
        if (currentIndex < 0 || currentIndex >= static_cast<int>(snapshots.size()))
            return;

        auto copy = snapshots[static_cast<size_t>(currentIndex)].createCopy();
        parameters.replaceState(copy);
    }

    static constexpr int snapshotDelayMs = 180;
    static constexpr size_t maxSnapshots = 32;

    juce::AudioProcessorValueTreeState& parameters;
    std::vector<juce::ValueTree> snapshots;
    int currentIndex = -1;
    bool pendingSnapshot = false;
    bool isRestoring = false;
};
} // namespace auralis::utils

