#ifndef BUTTON_H
#define BUTTON_H

class Button
{
private:
    uint8_t pin;
    bool previousState;
    void (*pressedCallback)();

public:
    Button(uint8_t pin, void (*pressedCallback)())
    {
        this->pin = pin;
        pinMode(this->pin, INPUT);

        this->previousState = false;

        this->pressedCallback = pressedCallback;
    }

    void update()
    {
        bool buttonState = digitalRead(this->pin);

        if (buttonState != this->previousState)
        {
            if (buttonState == true)
            {
                //button pressed callback
                (*this->pressedCallback)();
            }
            else if (buttonState == false)
            {
                //TODO released callback
            }
        }
        else if (buttonState == true)
        {
            //TODO held callback
        }

        this->previousState = buttonState;
    }
};

#endif