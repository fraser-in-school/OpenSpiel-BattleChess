#include "open_spiel/games/battle_chess/battle_board.h"

#include <cctype>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace open_spiel {
namespace battle_chess {

std::string ColorToString(Color c) {
  switch (c) {
    case Color::kBlack:
      return "black";
    case Color::kWhite:
      return "white";
    case Color::kEmpty:
      return "empty";
    default:
      SpielFatalError("Unknown color.");
      return "This will never return.";
  }
}

std::optional<PieceType> PieceTypeFromChar(char c) {
  switch (toupper(c)) {
    case 'K':
      return PieceType::kKing;
    case 'D':
      return PieceType::kDefender;
    case 'A':
      return PieceType::kAttacker;
    default:
      std::cerr << "Invalid piece type: " << c << std::endl;
      return std::nullopt;
  }
}

std::string PieceTypeToString(PieceType p, bool uppercase) {
  switch (p) {
    case PieceType::kEmpty:
      return ".";
    case PieceType::kKing:
      return uppercase ? "K" : "k";
    case PieceType::kDefender:
      return uppercase ? "D" : "d";
    case PieceType::kAttacker:
      return uppercase ? "A" : "a";
    default:
      SpielFatalError("Unknown piece.");
      return "This will never return.";
  }
}

std::string Piece::ToString() const {
  std::string base = PieceTypeToString(type);
  return color == Color::kWhite ? absl::AsciiStrToUpper(base)
                                : absl::AsciiStrToLower(base);
}
}  // namespace open_spiel
}  // namespace battle_chess
