#pragma once

enum class Response
{
    OK, // значение по умолчанию
    BACK, // кнопка назад
    REPLAY, // переиграть
    QUIT, // выход из игры
    CELL // клетка (если кто-то кликает по клетке)
};
