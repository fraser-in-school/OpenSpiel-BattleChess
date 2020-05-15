#ifndef THIRD_PARTY_OPEN_SPIEL_GAMES_IMPL_BATTLE_CHESS_BATTLE_BOARD_H_
#define THIRD_PARTY_OPEN_SPIEL_GAMES_IMPL_BATTLE_CHESS_BATTLE_BOARD_H_


#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "open_spiel/spiel_utils.h"

namespace open_spiel {
namespace battle_chess {

enum class PieceType : int8_t {
  kEmpty = 0,
  kKing = 1,
  kDefender = 2,
  kAttacker= 3,
};

// 下面几个函数都是仿照国际象棋的写法
static inline constexpr std::array<PieceType, 3> kPieceTypes = {
    {PieceType::kKing, PieceType::kDefender, PieceType::kAttacker}};

// In case all the pieces are represented in the same plane, these values are
// used to represent each piece type.
static inline constexpr std::array<float, 6> kPieceRepresentation = {
    {1, 0.8, 0.6}};

// Tries to parse piece type from char ('K', 'Q', 'R', 'B', 'N', 'P').
// Case-insensitive.
std::optional<PieceType> PieceTypeFromChar(char c);

// Converts piece type to one character strings - "K", "Q", "R", "B", "N", "P".
// p must be one of the enumerator values of PieceType.
std::string PieceTypeToString(PieceType p, bool uppercase = true);

// Piece 的结构， 仿照的是国际象棋的结构
struct Piece {
  bool operator==(const Piece& other) const {
    return type == other.type && color == other.color;
  }

  bool operator!=(const Piece& other) const { return !(*this == other); }

  std::string ToUnicode() const;
  std::string ToString() const;

  Color color;
  PieceType type;
};


class BattleChessBoard {
  public:
    BattleChessBoard();
    static std::optional<ChessBoard> BoardFromFEN(const std::string& fen);

    const Piece& at(Square sq) const { return board_[SquareToIndex_(sq)]; }

    void set_square(Square sq, Piece p);

    const std::array<Piece, kBoardSize * kBoardSize>& pieces() const {
      return board_;
    }

  private:
    std::array<Piece, kBoardSize * kBoardSize> board_;

}  // class BattleChessBoard
}  // namespace battle_chess
}  // namespace open_spiel
#endif  // THIRD_PARTY_OPEN_SPIEL_GAMES_IMPL_BATTLE_CHESS_BATTLE_BOARD_H_