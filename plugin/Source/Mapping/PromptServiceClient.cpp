#include "PromptServiceClient.h"

#include <juce_core/juce_core.h>

namespace auralis::mapping
{
namespace
{
constexpr const char* defaultBaseUrl = "http://127.0.0.1:8000";
}

PromptServiceClient::PromptServiceClient()
    : baseUrl(defaultBaseUrl)
{
}

PromptServiceClient::PromptServiceClient(juce::String baseUrlIn)
    : baseUrl(sanitiseBaseUrl(baseUrlIn))
{
    if (baseUrl.isEmpty())
        baseUrl = defaultBaseUrl;
}

void PromptServiceClient::setBaseUrl(juce::String newBaseUrl)
{
    const juce::ScopedLock lock(baseUrlLock);
    baseUrl = sanitiseBaseUrl(newBaseUrl);
    if (baseUrl.isEmpty())
        baseUrl = defaultBaseUrl;
}

juce::String PromptServiceClient::getBaseUrl() const
{
    const juce::ScopedLock lock(baseUrlLock);
    return baseUrl;
}

PromptServiceClient::Response PromptServiceClient::mapPrompt(const juce::String& prompt,
                                                             const juce::StringArray& styleHints,
                                                             std::optional<int> bpm,
                                                             std::optional<int> seed)
{
    const auto cacheKey = makeCacheKey(prompt, styleHints, bpm, seed);
    const auto cacheKeyStd = cacheKey.toStdString();

    {
        const juce::SpinLock::ScopedLockType lock(cacheLock);
        auto it = cache.find(cacheKeyStd);
        if (it != cache.end())
            return { true, 200, it->second, {}, true };
    }

    auto url = getBaseUrl();
    if (! url.endsWithChar('/'))
        url << '/';
    url << "map";

    juce::DynamicObject::Ptr payload = new juce::DynamicObject();
    payload->setProperty("prompt", prompt);

    if (styleHints.size() > 0)
    {
        juce::Array<juce::var> hintsVar;
        for (const auto& hint : styleHints)
            hintsVar.add(hint);
        payload->setProperty("styleHints", juce::var(hintsVar));
    }

    if (bpm.has_value())
        payload->setProperty("bpm", *bpm);

    if (seed.has_value())
        payload->setProperty("seed", *seed);

    const auto body = juce::JSON::toString(juce::var(payload.get()));

    juce::WebInputStream stream(juce::URL(url), true);
    stream.withExtraHeaders("Content-Type: application/json\r\n");
    stream.setRequestBody(body.toRawUTF8(), body.getNumBytesAsUTF8());

    if (! stream.connect(nullptr))
    {
        return { false, stream.getStatusCode(), {}, "Failed to connect to mapper service" };
    }

    const auto responseBody = stream.readEntireStreamAsString();
    const auto status = stream.getStatusCode();

    if (status < 200 || status >= 300)
    {
        return { false, status, responseBody, juce::String("HTTP ") + juce::String(status) };
    }

    {
        const juce::SpinLock::ScopedLockType lock(cacheLock);
        cache[cacheKeyStd] = responseBody;
    }

    return { true, status, responseBody, {}, false };
}

juce::String PromptServiceClient::sanitiseBaseUrl(const juce::String& url) const
{
    auto cleaned = url.trim();
    if (cleaned.endsWithChar('/'))
        cleaned = cleaned.dropLastCharacters(1);
    return cleaned;
}

juce::String PromptServiceClient::makeCacheKey(const juce::String& prompt,
                                               const juce::StringArray& styleHints,
                                               std::optional<int> bpm,
                                               std::optional<int> seed) const
{
    juce::StringArray parts;
    parts.add(prompt.trim().toLowerCase());

    juce::StringArray hintParts;
    for (const auto& hint : styleHints)
        hintParts.add(hint.trim().toLowerCase());
    hintParts.sort(true);
    parts.add(hintParts.joinIntoString("|"));

    parts.add(bpm.has_value() ? juce::String(*bpm) : "-");
    parts.add(seed.has_value() ? juce::String(*seed) : "-");

    return parts.joinIntoString("::");
}

} // namespace auralis::mapping
