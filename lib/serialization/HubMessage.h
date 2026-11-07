#ifndef HUB_MESSAGE_H
#define HUB_MESSAGE_H

#include <string>
#include <utility>
#include <vector>

class HubMessage
{
private:
    std::string type;
    bool hasRemainingSeconds = false;
    int remainingSeconds = 0;
    bool hasBuzzerUsername = false;
    std::string buzzerUsername;
    std::string title;
    bool hasQuestionIndex = false;
    int questionIndex = 0;
    bool hasChoices = false;
    std::vector<std::string> choices;

public:
    explicit HubMessage(std::string type) : type(std::move(type))
    {
    }

    const std::string &Type() const
    {
        return type;
    }

    HubMessage WithRemainingSeconds(int seconds) const
    {
        HubMessage copy = *this;
        copy.hasRemainingSeconds = true;
        copy.remainingSeconds = seconds;
        return copy;
    }

    bool HasRemainingSeconds() const
    {
        return hasRemainingSeconds;
    }

    int RemainingSeconds() const
    {
        return remainingSeconds;
    }

    HubMessage WithBuzzerUsername(std::string username) const
    {
        HubMessage copy = *this;
        copy.hasBuzzerUsername = true;
        copy.buzzerUsername = std::move(username);
        return copy;
    }

    bool HasBuzzerUsername() const
    {
        return hasBuzzerUsername;
    }

    const std::string &BuzzerUsername() const
    {
        return buzzerUsername;
    }

    HubMessage WithTitle(std::string questionTitle) const
    {
        HubMessage copy = *this;
        copy.title = std::move(questionTitle);
        return copy;
    }

    const std::string &Title() const
    {
        return title;
    }

    HubMessage WithQuestionIndex(int index) const
    {
        HubMessage copy = *this;
        copy.hasQuestionIndex = true;
        copy.questionIndex = index;
        return copy;
    }

    bool HasQuestionIndex() const
    {
        return hasQuestionIndex;
    }

    int QuestionIndex() const
    {
        return questionIndex;
    }

    HubMessage WithChoices(std::vector<std::string> questionChoices) const
    {
        HubMessage copy = *this;
        copy.hasChoices = true;
        copy.choices = std::move(questionChoices);
        return copy;
    }

    bool HasChoices() const
    {
        return hasChoices;
    }

    const std::vector<std::string> &Choices() const
    {
        return choices;
    }
};

#endif
