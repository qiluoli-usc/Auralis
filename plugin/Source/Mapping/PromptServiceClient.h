#pragma once

#include <juce_core/juce_core.h>

#include <optional>
#include <string>
#include <unordered_map>

namespace auralis::mapping
{
class PromptServiceClient
{
public:
    struct Response
    {
        bool ok { false };
        int statusCode { 0 };
        juce::String body;
        juce::String error;
        bool fromCache { false };
    };

    PromptServiceClient();
    explicit PromptServiceClient(juce::String baseUrl);

    void setBaseUrl(juce::String newBaseUrl);
    [[nodiscard]] juce::String getBaseUrl() const;

    Response mapPrompt(const juce::String& prompt,
                       const juce::StringArray& styleHints,
                       std::optional<int> bpm,
                       std::optional<int> seed);

private:
    [[nodiscard]] juce::String sanitiseBaseUrl(const juce::String& url) const;
    [[nodiscard]] juce::String makeCacheKey(const juce::String& prompt,
                                            const juce::StringArray& styleHints,
                                            std::optional<int> bpm,
                                            std::optional<int> seed) const;

    juce::String baseUrl;
    mutable juce::CriticalSection baseUrlLock;

    std::unordered_map<std::string, juce::String> cache;
    juce::SpinLock cacheLock;
};
}
