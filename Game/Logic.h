#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
  public:
    Logic(Board *board, Config *config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine (
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    vector<move_pos> find_best_turns(const bool color) // ищет лучшие ходы
    {
        next_move.clear(); // очищаем вектор следующих ходов
        next_best_state.clear(); // очищаем наше лучшее состояние

        find_first_best_turn(board->get_board(), color, -1, -1, 0); // нашли наилучшие ходы

        vector<move_pos> res;  // вектор результатов
        int state = 0; // начальное состояние
        // меняем состояние и записываем результат, пока можем ходить
        do {
            res.push_back(next_move[state]); 
            state = next_best_state[state];

        } while (state != -1 && next_move[state].x != -1);
        return res;  // возвращаем вектор результатов
    }

private:
    // ищет лучший первый ход
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
        double alpha = -1)
    {
        next_move.emplace_back(-1, -1, -1, -1); // изначально ход пустой
        next_best_state.push_back(-1); 
        // если текущее состояние не нулевое, то есть мы кого-то мы бьём
        if (state != 0) {
            find_turns(x, y, mtx);  // делаем ход
        }
        auto now_turns = turns;  // создаём копию переменной turns для рекурсивного подсчёта
        auto now_have_beats = have_beats;  // создаём копию переменной have_beats для рекурсивного подсчёта

        if (!now_have_beats && state != 0) {
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }
        double best_score = -1;  // наилучшее значение на данный момент из всех ходов
        for (auto turn : now_turns) {
            size_t new_state = next_move.size();  // новое состояние
            double score; // результат хода
            if (now_have_beats) { // если можем кого-то побить, делаем ход и продолжаем бить
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, new_state, best_score);
            }
            else { // если не можем никого побить, делаем ход и передаем его следующему игроку
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
            }
            if (score > best_score) { // если результат сделанного хода больше наилучшего
                best_score = score;  // меняем наилучший ход
                next_move[state] = turn;  // меняем следующий ход
                next_best_state[state] = (now_have_beats ? new_state : -1);  // меняем следующее состояние
            }
        }
        return best_score;
    }
    // ищет лучшие ходы
    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
        double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    {
        if (depth == Max_depth) { // если текущая глубина равна макисмальной
            return calc_score(mtx, (depth % 2 == color));  // возвращаем текущий результат
        }
        if (x != -1) { // у нас есть серия побитий
            find_turns(x, y, mtx);  // ищем ход
        }
        else {
            find_turns(color, mtx);  // ищем всевозможные ходы от цвета
        }

        auto now_turns = turns;  // создаём копию переменной turns для рекурсивного подсчёта
        auto now_have_beats = have_beats;  // создаём копию переменной have_beats для рекурсивного подсчёта
        if (!now_have_beats && x != -1) {
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        if (turns.empty()) { // если ходов нет
            return (depth % 2 ? 0 : INF);  // либо мы проиграли, либо соперник
        }

        double min_score = INF + 1;  // задаём минимум
        double max_score = -1;  // задаём максимум

        for (auto turn : now_turns) {
            double score;

            if (now_have_beats) {  // если можем побить бьём
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }
            else { // если не можем, то делаем ход и передаем его боту
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            min_score = min(min_score, score);
            max_score = max(max_score, score);

            if (depth % 2) {  // если ходим мы, то двигаем левую границу (максимизируем альфа)
                alpha = max(alpha, max_score);
            }
            else {  // если ходит бот, то двигаем правую границу (минимимзируем бета)
                beta = min(beta, min_score);
            }
            if (optimization != "O0" && alpha > beta) { // если без оптимизации и левая граница больше правой, то эту вершину не берём
                break;  // выходим из цикла
            }
            if (optimization == "O2" && alpha == beta) { // если оптимизацией нестрогая
                return (depth % 2 ? max_score + 1 : min_score - 1);  // результаты не будут выбраны выше как оптимумы
            }
        }
        return (depth % 2 ? max_score : min_score);  // возвращаем результат
    }
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const  // ход игрока или бота
    {
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0;
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];
        mtx[turn.x][turn.y] = 0;
        return mtx;
    }

    double calc_score(const vector<vector<POS_T>> &mtx, const bool first_bot_color) const  // подсчёт очков
    {
        // color - who is max player
        double w = 0, wq = 0, b = 0, bq = 0;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1);
                wq += (mtx[i][j] == 3);
                b += (mtx[i][j] == 2);
                bq += (mtx[i][j] == 4);
                if (scoring_mode == "NumberAndPotential")
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }
        if (w + wq == 0)
            return INF;
        if (b + bq == 0)
            return 0;
        int q_coef = 4;
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }
        return (b + bq * q_coef) / (w + wq * q_coef);
    }
    

public:
    void find_turns(const bool color) // ищет ходы с текущей доски
    {
        find_turns(color, board->get_board()); 
    }

    void find_turns(const POS_T x, const POS_T y) // ищет ходы с той доски, которую мы передали
    {
        find_turns(x, y, board->get_board());
    }

private:
    void find_turns(const bool color, const vector<vector<POS_T>> &mtx) // приветный метод find_turns, который вызывается другими методами find_turns
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        // проходимя по всем клеткам доски
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color) // если клетка совпадает с цветом
                {
                    find_turns(i, j, mtx); // ищем возможные ходы
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }

    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>> &mtx) // ищет возможные ходы от текущей позиции
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y];
        // check beats
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        // check other turns
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            {
                POS_T i = ((type % 2) ? x - 1 : x + 1);
                for (POS_T j = y - 1; j <= y + 1; j += 2)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                        continue;
                    turns.emplace_back(x, y, i, j);
                }
                break;
            }
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

  public:
    vector<move_pos> turns; // вектор ходов
    bool have_beats; // есть ли у нас побитие
    int Max_depth; // уровень бота

  private:
    default_random_engine rand_eng; // генерация случайных чисел
    string scoring_mode; // режим подсчёта очков
    string optimization; // оптимизация ходов
    vector<move_pos> next_move; // вектор следующих ходов
    vector<int> next_best_state; // следующей наилучшее состояние
    Board *board; // указатель на объект класса "доска"
    Config *config; // указатель на объект класса "конфигурация"
};
