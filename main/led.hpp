#pragma once

class Led {
    public:
        Led();

    public:
        void init();
        void setLed(bool led[4]);
        bool* getLed() { return m_led; }

    private:
        void ledRef();

    private:
        bool m_led[4];
};
