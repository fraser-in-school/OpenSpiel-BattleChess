// Copyright 2019 DeepMind Technologies Ltd. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef THIRD_PARTY_OPEN_SPIEL_GAMES_BREAKTHROUGH_H_
#define THIRD_PARTY_OPEN_SPIEL_GAMES_BREAKTHROUGH_H_

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"

// Breakthrough, a game used in the general game-play competition
// http://en.wikipedia.org/wiki/Breakthrough_%28board_game%29
//
// Parameters:
//       "columns"    int     number of columns on the board   (default = 8)
//       "rows"       int     number of rows on the board      (default = 8)

namespace open_spiel {
namespace battle_chess {

inline constexpr int kNumPlayers = 2;
inline constexpr int kBlackPlayerId = 1;
inline constexpr int kWhitePlayerId = 0;
inline constexpr int kCellStates =
    1 + 3 /* 有三种类型的棋子 */ * kNumPlayers;  // player 0, player 1, empty.
inline constexpr int kDefaultRows = 5;
inline constexpr int kDefaultColumns = 5;

// State of a cell.
enum class CellState {
  kEmpty,
  kBlackKing,
  kBlackDefender,
  kBlackAttacker,
  kWhiteKing,
  kWhiteDefender,
  kWhiteAttacker
};

//enum class Color {
//  kBlack,
//  kWhite
//}
struct Piece {
  int row;
  int col;
  CellState pieceType;
};

class BattleChessState : public State {
 public:
  explicit BattleChessState(std::shared_ptr<const Game> game);
  Player CurrentPlayer() const override;
  std::string ActionToString(Player player, Action action) const override;
  std::string ToString() const override;
  bool IsTerminal() const override;
  std::vector<double> Returns() const override;
  std::string ObservationString(Player player) const override;
  void ObservationTensor(Player player,
                         std::vector<double>* values) const override;
  std::unique_ptr<State> Clone() const override;
  void UndoAction(Player player, Action action) override;

  bool InBounds(int r, int c) const;
  void SetBoard(int r, int c, CellState cs) { board_[r * cols_ + c] = cs; }
  void AddPiece(int r, int c, CellState state);
  void DeletePiece(int r, int c, CellState state);
  // 用于清空棋子
  void InitPieces(int color);
  CellState board(int row, int col) const { return board_[row * cols_ + col]; }

  int rows() const { return rows_; }
  int cols() const { return cols_; }
  std::vector<Action> LegalActions() const override;
  std::string Serialize() const override;

  void getPiecesStatus() const;
 protected:
  void DoApplyAction(Action action) override;

 private:
  int observation_plane(int r, int c) const;

  // Fields sets to bad/invalid values. Use Game::NewInitialState().
  Player cur_player_ = kInvalidPlayer;
  int winner_ = kInvalidPlayer;
  int total_moves_ = -1;
  std::vector<Piece> whitePieces;
  std::vector<Piece> blackPieces;
  int rows_ = kDefaultRows;
  int cols_ = kDefaultColumns;
  std::vector<CellState> board_;  // for (row,col) we use row*cols_ + col.
};

class BattleChessGame : public Game {
 public:
  explicit BattleChessGame(const GameParameters& params);
  int NumDistinctActions() const override;
  std::unique_ptr<State> NewInitialState() const override {
    return std::unique_ptr<State>(
        new BattleChessState(shared_from_this()));
  }
  int NumPlayers() const override { return kNumPlayers; }
  double MinUtility() const override { return -1; }
  double UtilitySum() const override { return 0; }
  double MaxUtility() const override { return 1; }
  std::shared_ptr<const Game> Clone() const override {
    return std::shared_ptr<const Game>(new BattleChessGame(*this));
  }
  std::vector<int> ObservationTensorShape() const override {
    return {kCellStates, rows_, cols_};
  }

  // Each piece must move forward from its current position, so the second last
  // row on the opponent's side is a distance of rows_ - 2 for the front row,
  // and rows_ - 1 for the back row (= 2*rows_ - 3). This can be done for each
  // column, and for both players, and there is one final move to step onto the
  // last winning row. As such, the formula for maximum game length is:

  // gameLength 并没有经过计算现在采用的是国际象棋的数据
  int MaxGameLength() const override {
    return 1000;
  }
  std::unique_ptr<State> DeserializeState(
      const std::string& str) const override;

 private:
  int rows_ = kDefaultRows;
  int cols_ = kDefaultColumns;
};

inline std::ostream& operator<<(std::ostream& stream, const CellState& state) {
  switch (state) {
    case CellState::kBlackKing:
      return stream << "BlackKing";
    case CellState::kBlackDefender:
      return stream << "BlackDefender";
    case CellState::kBlackAttacker:
      return stream << "BlackAttacker";
    case CellState::kWhiteKing:
      return stream << "WhiteKing";
    case CellState::kWhiteDefender:
      return stream << "WhiteDefender";
    case CellState::kWhiteAttacker:
      return stream << "WhiteAttacker";
    case CellState::kEmpty:
      return stream << "Empty";
    default:
      SpielFatalError("Unknown cell state");
  }
}

}  // namespace battle_chess
}  // namespace open_spiel

#endif  // THIRD_PARTY_OPEN_SPIEL_GAMES_BREAKTHROUGH_H_
