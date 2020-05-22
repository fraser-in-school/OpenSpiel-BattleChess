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

#include "open_spiel/games/battle_chess.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "open_spiel/game_parameters.h"
#include "open_spiel/utils/tensor_view.h"

namespace open_spiel {
namespace battle_chess {
namespace {

// Number of unique directions each piece can take.
constexpr int kNumDirections = 6;

// Numbers of rows needed to have 2 full rows of pieces.
constexpr int kNumRowsForFullPieces = 6;

// Direction offsets for black, then white.
constexpr std::array<int, kNumDirections> kDirRowOffsets = {
    {1, 1, 1, -1, -1, -1}};

constexpr std::array<int, kNumDirections> kDirColOffsets = {
    {-1, 0, 1, -1, 0, 1}};

// Facts about the game
const GameType kGameType{/*short_name=*/"battle_chess",
                         /*long_name=*/"BattleChess",
                         GameType::Dynamics::kSequential,
                         GameType::ChanceMode::kDeterministic,
                         GameType::Information::kPerfectInformation,
                         GameType::Utility::kZeroSum,
                         GameType::RewardModel::kTerminal,
                         /*max_num_players=*/2,
                         /*min_num_players=*/2,
                         /*provides_information_state_string=*/false,
                         /*provides_information_state_tensor=*/false,
                         /*provides_observation_string=*/true,
                         /*provides_observation_tensor=*/true,
                         /*parameter_specification=*/{}
};

std::shared_ptr<const Game> Factory(const GameParameters& params) {
  return std::shared_ptr<const Game>(new BattleChessGame(params));
}

REGISTER_SPIEL_GAME(kGameType, Factory);

int StateToColor(CellState state){
  switch (state) {
    case CellState::kBlackKing:
      return 1;
    case CellState::kBlackDefender:
      return 1;
    case CellState::kBlackAttacker:
      return 1;
    case CellState::kWhiteKing:
      return 0;
    case CellState::kWhiteDefender:
      return 0;
    case CellState::kWhiteAttacker:
      return 0;
    case CellState::kEmpty:
      return -1;
  }
}

std::string CellToString(CellState state) {
  switch (state) {
    case CellState::kBlackKing:
      return "k";
    case CellState::kBlackDefender:
      return "d";
    case CellState::kBlackAttacker:
      return "a";
    case CellState::kWhiteKing:
      return "K";
    case CellState::kWhiteDefender:
      return "D";
    case CellState::kWhiteAttacker:
      return "A";
    case CellState::kEmpty:
      return ".";
    default:
      SpielFatalError("Unrecognized cell state");
  }
}


std::string RowLabel(int rows, int row) {
  std::string label = "";
  label += static_cast<char>('1' + (rows - 1 - row));
  return label;
}

std::string ColLabel(int col) {
  std::string label = "";
  label += static_cast<char>('a' + col);
  return label;
}

// 根据capture 的值判断被吃掉的棋子
std::string PieceName(int capture) {
  std::string name = "";
  if(capture == 0){
    name += "-";
  } else if(capture == 1) {
    name += "A";
  } else if(capture == 2) {
    name += "D";
  } else if(capture == 3) {
    name += "K";
  } else if(capture == 4) {
    name += "a";
  } else if(capture == 5) {
    name += "d";
  } else if(capture == 6) {
    name += "k";
  }

  return name;
}

// 根据capture 的值判断被吃掉的棋子类型
CellState StateType(int capture) {
  if(capture == 0){
    return CellState::kEmpty;
  } else if(capture == 1) {
    return CellState::kWhiteAttacker;
  } else if(capture == 2) {
    return CellState::kWhiteDefender;
  } else if(capture == 3) {
    return CellState::kWhiteKing;
  } else if(capture == 4) {
    return CellState::kBlackAttacker;
  } else if(capture == 5) {
    return CellState::kBlackDefender;
  } else if(capture == 6) {
    return CellState::kBlackKing;
  }
  
  SpielFatalError("wrong value");
}

// 根据被吃掉的棋子来获得 capture 的值
int getCaptureValue(CellState state){
  if(state == CellState::kWhiteAttacker) {
    return 1;
  } else if(state == CellState::kWhiteDefender) {
    return 2;
  } else if(state == CellState::kWhiteKing) {
    return 3;
  } else if(state == CellState::kBlackAttacker) {
    return 4;
  } else if(state == CellState::kBlackDefender) {
    return 5;
  } else if(state == CellState::kBlackKing) {
    return 6;
  } else {
    return 0;
  }
}



}  // namespace

BattleChessState::BattleChessState(std::shared_ptr<const Game> game)
    : State(game){
  SPIEL_CHECK_GT(rows_, 1);
  SPIEL_CHECK_GT(cols_, 1);

  board_ = std::vector<CellState>(rows_ * cols_, CellState::kEmpty);
  // 放置黑棋
  SetBoard(0, 0, CellState::kBlackAttacker);
  AddPiece(0, 0, CellState::kBlackAttacker);

  SetBoard(0, 1, CellState::kBlackDefender);
  AddPiece(0, 1, CellState::kBlackDefender);

  SetBoard(0, 2, CellState::kBlackKing);
  AddPiece(0, 2, CellState::kBlackKing);

  SetBoard(0, 3, CellState::kBlackDefender);
  AddPiece(0, 3, CellState::kBlackDefender);

  SetBoard(0, 4, CellState::kBlackAttacker);
  AddPiece(0, 4, CellState::kBlackAttacker);

  // 放置白棋
  SetBoard(4, 0, CellState::kWhiteAttacker);
  AddPiece(4, 0, CellState::kWhiteAttacker);

  SetBoard(4, 1, CellState::kWhiteDefender);
  AddPiece(4, 1, CellState::kWhiteDefender);

  SetBoard(4, 2, CellState::kWhiteKing);
  AddPiece(4, 2, CellState::kWhiteKing);

  SetBoard(4, 3, CellState::kWhiteDefender);
  AddPiece(4, 3, CellState::kWhiteDefender);

  SetBoard(4, 4, CellState::kWhiteAttacker);
  AddPiece(4, 4, CellState::kWhiteAttacker);

  winner_ = kInvalidPlayer;
  cur_player_ = 0;
  total_moves_ = 0;
}

int BattleChessState::CurrentPlayer() const {
  if (IsTerminal()) {
    return kTerminalPlayerId;
  } else {
    return cur_player_;
  }
}

void BattleChessState::AddPiece(int r, int c, CellState state){
  Piece piece;
  piece.row = r;
  piece.col = c;
  piece.pieceType = state;
//   std::cout << "in AddPiece() func" << std::endl;
//   std::cout << r << "-" << c << std::endl;
//   std::cout << CellToString(piece.pieceType) << " :" << piece.row << "-" << piece.col << std::endl;
  if (StateToColor(state) == 0) {
    whitePieces.push_back(piece);
  } else {
    blackPieces.push_back(piece);
  }
}

// 先根据 state 来判断颜色
// 再走上同的Pieces 数组里面匹配位置相同的， 若相同， 则删除该棋子
void BattleChessState::DeletePiece(int r, int c, CellState state){
  // 检查是否有匹配的棋子的flag
  bool flag = false;
  if (StateToColor(state) == 0) {
    for(auto piece = whitePieces.begin(); piece != whitePieces.end(); piece ++) {
      if(piece->row == r && piece->col == c){
        whitePieces.erase(piece);
        flag = true;
        break;
      }
    }
  } else {
    for(auto piece = blackPieces.begin(); piece != blackPieces.end(); piece ++) {
      if(piece->row == r && piece->col == c){
        blackPieces.erase(piece);
        flag = true;
        break;
      }
    }
  }
  if(!flag){
    SpielFatalError("can not find piece in" + std::to_string(r) + " : " + std::to_string(c));
  }
}

// 将所有棋子都清空
void BattleChessState::InitPieces(int color){
  if (color == 0) {
    whitePieces.clear();
  } else if(color == 1){
    blackPieces.clear();
  }
}

// 输出保存棋子的状态
void BattleChessState::getPiecesStatus() const {
    std::cout<< "white piece: " << std::endl;
    for(int i = 0; i < whitePieces.size(); i ++) {
      std::cout<< CellToString(whitePieces[i].pieceType) << " :" << whitePieces[i].row << "-" << whitePieces[i].col << std::endl;
    }

    std::cout<< "black piece: " << std::endl;
    for(int i = 0; i < blackPieces.size(); i ++) {
      std::cout<< CellToString(blackPieces[i].pieceType) << " :" << blackPieces[i].row << "-" << blackPieces[i].col << std::endl;
    }

    std::cout << "board" << std::endl;
    for(int i = 0; i < 5; i ++){
      for(int j = 0; j < 5; j ++)
        std::cout << CellToString(board(i, j));
      std::cout << std::endl;
    }
  }

// 根据 king 的位置推断出 king 的落子
// 返回位置是起始位置以及落子位置
// 只判断是不是超出了边界而不判断是不是有同种颜色的棋子
void KingAction(int r1, int c1, std::vector<std::vector<int>> &kingActions){
  std::vector<int> target(5, 0);
  int r2 = 0;
  int c2 = 0;
  // 方向数组
  // 前后左右四个方向
  int dir[4][2] = {
    {0, 1},
    {0, -1},
    {1, 0},
    {-1, 0},
  };
  for(int i = 0; i < 4; i++) {
    target[0] = r1;
    target[1] = c1;
    target[2] = r1 + dir[i][0];
    target[3] = c1 + dir[i][1];
    // 这里检查 落子位置是不是在界内
    kingActions.push_back(target);
  }
}

// 根据 Defender 的位置推断出 Defender 的落子
// 返回位置是起始位置以及落子位置
// 只判断是不是超出了边界而不判断是不是有同种颜色的棋子
void DefenderAction(int r1, int c1, std::vector<std::vector<int>> &defenderActions){
  std::vector<int> target(5, 0);
  int r2 = 0;
  int c2 = 0;

  // 方向数组1
  // 前后左右四个方向
  int dir1[4][2] = {
    {0, 1},
    {0, -1},
    {1, 0},
    {-1, 0},
  };

  // 方向数组2
  // 4个斜角方向
  int dir2[4][2] = {
    {1, 1},
    {1, -1},
    {-1, -1},
    {-1, 1},
  };

  // 前后左右四个方向，只能移动不能吃
  for(int i = 0; i < 4; i++) {
    target[0] = r1;
    target[1] = c1;
    target[2] = r1 + dir1[i][0];
    target[3] = c1 + dir1[i][1];
    defenderActions.push_back(target);
  }

  // 四个斜角方向只能吃，不能干移动
  // capture 值设为 -1 是为了后面的检查
  target[4] = -1;
  for(int i = 0; i < 4; i++) {
    target[0] = r1;
    target[1] = c1;
    target[2] = r1 + dir2[i][0];
    target[3] = c1 + dir2[i][1];
    defenderActions.push_back(target);
  }
}

// 根据 Attacker 的位置推断出 Attacker 的落子
// 返回位置是起始位置以及落子位置
// 只判断是不是超出了边界而不判断是不是有同种颜色的棋子
void AttackerAction(int r1, int c1, std::vector<std::vector<int>> &attackerActions){
  std::vector<int> target(5, 0);
  int r2 = 0;
  int c2 = 0;

  // 方向数组1
  // 前后左右四个方向
  int dir1[4][2] = {
    {0, 1},
    {0, -1},
    {1, 0},
    {-1, 0},
  };

  // 方向数组2
  // 4个斜角方向
  int dir2[4][2] = {
    {1, 1},
    {1, -1},
    {-1, -1},
    {-1, 1},
  };

  // 四个斜角方向只能移动不能吃
  for(int i = 0; i < 4; i++) {
    target[0] = r1;
    target[1] = c1;
    target[2] = r1 + dir2[i][0];
    target[3] = c1 + dir2[i][1];
    // 这里检查 落子位置是不是在界内
    attackerActions.push_back(target);
  }

  // 前后左右四个方向，只能吃不能移动
  // capture 值设为 -1 是为了后面的检查
  target[4] = -1;
  for(int i = 0; i < 4; i++) {
    target[0] = r1;
    target[1] = c1;
    target[2] = r1 + dir1[i][0];
    target[3] = c1 + dir1[i][1];
    // 这里检查 落子位置是不是在界内
    attackerActions.push_back(target);
  }
}

void BattleChessState::DoApplyAction(Action action) {
  std::vector<int> values(5, -1);
  UnrankActionMixedBase(action, {rows_, cols_, rows_, cols_, 9}, &values);
  int r1 = values[0];
  int c1 = values[1];
  int r2 = values[2];
  int c2 = values[3];
  int capture = values[4];

  SPIEL_CHECK_TRUE(InBounds(r1, c1));
  SPIEL_CHECK_TRUE(InBounds(r2, c2));
  //   std::cout<<r1<< "-"<< c1<< "/" << r2 << "-" << c2 <<std::endl;
  // capture == 0 只是移动棋子，不吃对方棋子
  // capture > 0 吃棋行为
  // 1,2,3 白方 A,D,K
  // 4,5,6 黑方 a,d,k
  if (capture > 0) {
    // 不能吃掉同种棋
    SPIEL_CHECK_EQ(1 - StateToColor(board(r2, c2)), StateToColor(board(r1, c1)));

    //删除被吃掉的棋
    DeletePiece(r2, c2, board(r2, c2));

    // 判断胜负
    if(board(r2, c2) == CellState::kBlackKing) {
      winner_ = 0;
    }
    if(board(r2, c2) == CellState::kWhiteKing) {
      winner_ = 1;
    }
  }

  // 下面的两个判断是为了变更棋子的位置
  DeletePiece(r1, c1, board(r1, c1));
  AddPiece(r2, c2, board(r1, c1));

  SetBoard(r2, c2, board(r1, c1));
  SetBoard(r1, c1, CellState::kEmpty);

  // getPiecesStatus();
  // 切换 player
  cur_player_ = NextPlayerRoundRobin(cur_player_, kNumPlayers);
  total_moves_++;

  if(total_moves_ >= 1000) {
    winner_ = 1;
  }
}

std::string BattleChessState::ActionToString(Player player,
                                              Action action) const {
  std::vector<int> values(5, -1);
  UnrankActionMixedBase(action, {rows_, cols_, rows_, cols_, 9}, &values);
  int r1 = values[0];
  int c1 = values[1];
  int r2 = values[2];
  int c2 = values[3];
  int capture = values[4];

  std::string action_string = "";
  absl::StrAppend(&action_string, ColLabel(c1));
  absl::StrAppend(&action_string, RowLabel(rows_, r1));
  absl::StrAppend(&action_string, ColLabel(c2));
  absl::StrAppend(&action_string, RowLabel(rows_, r2));
  absl::StrAppend(&action_string, PieceName(capture));

  return action_string;
}

std::vector<Action> BattleChessState::LegalActions() const {
  std::vector<Action> movelist;
  if (IsTerminal()) return movelist;
  const Player player = CurrentPlayer();
  std::vector<int> action_bases = {rows_, cols_, rows_, cols_, 9};
  std::vector<int> action_values = {0, 0, 0, 0, 0};

  if (player == 0 ) {
    for(int i = 0; i < whitePieces.size(); i ++) {

      // King 的合法动作
      if(whitePieces[i].pieceType == CellState::kWhiteKing) {
        std::vector<std::vector<int>> kingActions;

        // 检查棋子位置是否合法
        SPIEL_CHECK_TRUE(InBounds(whitePieces[i].row, whitePieces[i].col));

        // 根据king的位置来推断落子位置
        // 只进行了是否在边界内的检查
        KingAction(whitePieces[i].row, whitePieces[i].col, kingActions);

        // 对每个待定动作做进一步检查
        // 如果合法， 则加入 movelist
        for(int i = 0; i < kingActions.size(); i ++) {

          // 落子位置是不是在界内
          if(!InBounds(kingActions[i][2], kingActions[i][3])) continue;

          // 白色为0， 黑色为1， 空白为 -1
          // 颜色的值必须不同
          if(StateToColor(board(kingActions[i][0], kingActions[i][1])) != StateToColor(board(kingActions[i][2], kingActions[i][3]))) {
            // 如果落子位置不为空， 则需要根据棋子类型得到capture的值
            if (StateToColor(board(kingActions[i][2], kingActions[i][3])) != -1) {
              kingActions[i][4] = getCaptureValue(board(kingActions[i][2], kingActions[i][3]));
            }
            movelist.push_back(
                  RankActionMixedBase(action_bases, kingActions[i]));
          }
        }
      }  // if King

      // Defender 的合法动作
      if(whitePieces[i].pieceType == CellState::kWhiteDefender) {
        std::vector<std::vector<int>> defenderActions;

        // 检查棋子位置是否合法
        SPIEL_CHECK_TRUE(InBounds(whitePieces[i].row, whitePieces[i].col));

        // 根据king的位置来推断落子位置
        // 只进行了是否在边界内的检查
        DefenderAction(whitePieces[i].row, whitePieces[i].col, defenderActions);

        // 对每个待定动作做进一步检查
        // 如果合法， 则加入 movelist
        for(int i = 0; i < defenderActions.size(); i ++) {

          // 落子位置是不是在界内
          if(!InBounds(defenderActions[i][2], defenderActions[i][3])) continue;

          // capture 的值为0的时候，则需要落子位置为空
          // capture 的值为-1的时候，则需要落子位置为另一种颜色的棋
          if(defenderActions[i][4] == 0) {
            if(StateToColor(board(defenderActions[i][2], defenderActions[i][3])) == -1) {
              movelist.push_back(
                  RankActionMixedBase(action_bases, defenderActions[i]));
            }
          }

          // 需要吃子
          if(defenderActions[i][4] == -1) {
            if(StateToColor(board(defenderActions[i][2], defenderActions[i][3])) == 1) {
              // 需要变更capture的值
              defenderActions[i][4] = getCaptureValue(board(defenderActions[i][2], defenderActions[i][3]));
              movelist.push_back(
                  RankActionMixedBase(action_bases, defenderActions[i]));
            }
          }
        }
      }  // if Defender

      // Attacker 的合法动作
      if(whitePieces[i].pieceType == CellState::kWhiteAttacker) {
        std::vector<std::vector<int>> attackerActions;

        // 检查棋子位置是否合法
        SPIEL_CHECK_TRUE(InBounds(whitePieces[i].row, whitePieces[i].col));

        // 根据king的位置来推断落子位置
        // 只进行了是否在边界内的检查
        AttackerAction(whitePieces[i].row, whitePieces[i].col, attackerActions);

        // 对每个待定动作做进一步检查
        // 如果合法， 则加入 movelist
        for(int i = 0; i < attackerActions.size(); i ++) {

          // 落子位置是不是在界内
          if(!InBounds(attackerActions[i][2], attackerActions[i][3])) continue;

          // capture 的值为0的时候，则需要落子位置为空
          // capture 的值为-1的时候，则需要落子位置为另一种颜色的棋
          if(attackerActions[i][4] == 0) {
            if(StateToColor(board(attackerActions[i][2], attackerActions[i][3])) == -1) {
              movelist.push_back(
                  RankActionMixedBase(action_bases, attackerActions[i]));
            }
          }

          // 需要吃子
          if(attackerActions[i][4] == -1) {
            if(StateToColor(board(attackerActions[i][2], attackerActions[i][3])) == 1) {
              // 需要变更capture的值
              attackerActions[i][4] = getCaptureValue(board(attackerActions[i][2], attackerActions[i][3]));
              movelist.push_back(
                  RankActionMixedBase(action_bases, attackerActions[i]));
            }
          }
        }
      }  // if Attacker
    } // for
  } // player == 0

  if (player == 1 ) {
    for(int i = 0; i < blackPieces.size(); i ++) {

      // King 的合法动作
      if(blackPieces[i].pieceType == CellState::kBlackKing) {
        std::vector<std::vector<int>> kingActions;

        // 检查棋子位置是否合法
        SPIEL_CHECK_TRUE(InBounds(blackPieces[i].row, blackPieces[i].col));

        // 根据king的位置来推断落子位置
        // 只进行了是否在边界内的检查
        KingAction(blackPieces[i].row, blackPieces[i].col, kingActions);

        // 对每个待定动作做进一步检查
        // 如果合法， 则加入 movelist
        for(int i = 0; i < kingActions.size(); i ++) {

          // 落子位置是不是在界内
          if(!InBounds(kingActions[i][2], kingActions[i][3])) continue;

          // 白色为0， 黑色为1， 空白为 -1
          // 颜色的值必须不同
          if(StateToColor(board(kingActions[i][0], kingActions[i][1])) != StateToColor(board(kingActions[i][2], kingActions[i][3]))) {
            // 如果落子位置不为空， 则需要根据棋子类型得到capture的值
            if (StateToColor(board(kingActions[i][2], kingActions[i][3])) != -1) {
              kingActions[i][4] = getCaptureValue(board(kingActions[i][2], kingActions[i][3]));
            }
            movelist.push_back(
                  RankActionMixedBase(action_bases, kingActions[i]));
          }
        }
      }  // if King

      // Defender 的合法动作
      if(blackPieces[i].pieceType == CellState::kBlackDefender) {
        std::vector<std::vector<int>> defenderActions;

        // 检查棋子位置是否合法
        SPIEL_CHECK_TRUE(InBounds(blackPieces[i].row, blackPieces[i].col));

        // 根据king的位置来推断落子位置
        // 只进行了是否在边界内的检查
        DefenderAction(blackPieces[i].row, blackPieces[i].col, defenderActions);

        // 对每个待定动作做进一步检查
        // 如果合法， 则加入 movelist
        for(int i = 0; i < defenderActions.size(); i ++) {

          // 落子位置是不是在界内
          if(!InBounds(defenderActions[i][2], defenderActions[i][3])) continue;

          // capture 的值为0的时候，则需要落子位置为空
          // capture 的值为-1的时候，则需要落子位置为另一种颜色的棋
          if(defenderActions[i][4] == 0) {
            if(StateToColor(board(defenderActions[i][2], defenderActions[i][3])) == -1) {
              movelist.push_back(
                  RankActionMixedBase(action_bases, defenderActions[i]));
            }
          }

          // 需要吃子
          if(defenderActions[i][4] == -1) {
            if(StateToColor(board(defenderActions[i][2], defenderActions[i][3])) == 0) {
              // 需要变更capture的值
              defenderActions[i][4] = getCaptureValue(board(defenderActions[i][2], defenderActions[i][3]));
              movelist.push_back(
                  RankActionMixedBase(action_bases, defenderActions[i]));
            }
          }
        }
      }  // if Defender

      // Attacker 的合法动作
      if(blackPieces[i].pieceType == CellState::kBlackAttacker) {
        std::vector<std::vector<int>> attackerActions;

        // 检查棋子位置是否合法
        SPIEL_CHECK_TRUE(InBounds(blackPieces[i].row, blackPieces[i].col));

        // 根据king的位置来推断落子位置
        // 只进行了是否在边界内的检查
        AttackerAction(blackPieces[i].row, blackPieces[i].col, attackerActions);

        // 对每个待定动作做进一步检查
        // 如果合法， 则加入 movelist
        for(int i = 0; i < attackerActions.size(); i ++) {

          // 落子位置是不是在界内
          if(!InBounds(attackerActions[i][2], attackerActions[i][3])) continue;

          // capture 的值为0的时候，则需要落子位置为空
          // capture 的值为-1的时候，则需要落子位置为另一种颜色的棋
          if(attackerActions[i][4] == 0) {
            if(StateToColor(board(attackerActions[i][2], attackerActions[i][3])) == -1) {
              movelist.push_back(
                  RankActionMixedBase(action_bases, attackerActions[i]));
            }
          }

          // 需要吃子
          if(attackerActions[i][4] == -1) {
            if(StateToColor(board(attackerActions[i][2], attackerActions[i][3])) == 0) {
              // 需要变更capture的值
              attackerActions[i][4] = getCaptureValue(board(attackerActions[i][2], attackerActions[i][3]));
              movelist.push_back(
                  RankActionMixedBase(action_bases, attackerActions[i]));
            }
          }
        }
      }  // if Attacker
    } // for
  } // player == 1

//   std::vector<int> val(5, -1);
//   for (auto ac : movelist) {
//     UnrankActionMixedBase(ac, {rows_, cols_, rows_, cols_, 9}, &val);
//     std::cout << val[0] << " " <<val[1] << " " << val[2] << " " << val[3] << " " << val[4] << " " << "\n";
//   }
  std::sort(movelist.begin(), movelist.end());
  if(movelist.size() == 0) {
    getPiecesStatus();
  }
  return movelist;
}

bool BattleChessState::InBounds(int r, int c) const {
  return (r >= 0 && r < rows_ && c >= 0 && c < cols_);
}

std::string BattleChessState::ToString() const {
  std::string result = "";

  for (int r = 0; r < rows_; r++) {
    absl::StrAppend(&result, RowLabel(rows_, r));

    for (int c = 0; c < cols_; c++) {
      absl::StrAppend(&result, CellToString(board(r, c)));
    }

    result.append("\n");
  }

  absl::StrAppend(&result, " ");
  for (int c = 0; c < cols_; c++) {
    absl::StrAppend(&result, ColLabel(c));
  }
  absl::StrAppend(&result, "\n");

  return result;
}

int BattleChessState::observation_plane(int r, int c) const {
  int plane = -1;
  switch (board(r, c)) {
    case CellState::kBlackKing:
      plane = 0;
      break;
    case CellState::kBlackDefender:
      plane = 1;
      break;
    case CellState::kBlackAttacker:
      plane = 2;
      break;
    case CellState::kWhiteKing:
      plane = 3;
      break;
    case CellState::kWhiteDefender:
      plane = 4;
      break;
    case CellState::kWhiteAttacker:
      plane = 5;
      break;
    case CellState::kEmpty:
      plane = 6;
      break;
  }

  return plane;
}

bool BattleChessState::IsTerminal() const {
  return (winner_ >= 0);
}

std::vector<double> BattleChessState::Returns() const {
  if (winner_ == 0) {
    return {1.0, -1.0};
  } else if (winner_ == 1) {
    return {-1.0, 1.0};
  } else {
    return {0.0, 0.0};
  }
}

std::string BattleChessState::ObservationString(Player player) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, num_players_);
  return ToString();
}

// ??
void BattleChessState::ObservationTensor(Player player,
                                          std::vector<double>* values) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, num_players_);

  TensorView<3> view(values, {kCellStates, rows_, cols_}, true);

  for (int r = 0; r < rows_; r++) {
    for (int c = 0; c < cols_; c++) {
      int plane = observation_plane(r, c);
      SPIEL_CHECK_TRUE(plane >= 0 && plane < kCellStates);
      view[{plane, r, c}] = 1.0;
    }
  }
}

void BattleChessState::UndoAction(Player player, Action action) {
  std::vector<int> values(5, -1);
  UnrankActionMixedBase(action, {rows_, cols_, rows_, cols_, 9}, &values);
  int r1 = values[0];
  int c1 = values[1];
  int r2 = values[2];
  int c2 = values[3];
  int capture = values[4];

  SPIEL_CHECK_TRUE(InBounds(r1, c1));
  SPIEL_CHECK_TRUE(InBounds(r2, c2));

  cur_player_ = PreviousPlayerRoundRobin(cur_player_, 2);
  total_moves_--;

  // Undo win status.
  winner_ = kInvalidPlayer;

  // Move back the piece, and put back the opponent's piece if necessary.
  // The move is (r1, c1) -> (r2, c2) where r is row and c is column.
  SetBoard(r1, c1, board(r2, c2));
  SetBoard(r2, c2, CellState::kEmpty);
  
  // 如果吃子了， 则需要在r2, c2 上放置一个棋子
  if (capture > 0) {
    CellState state = StateType(capture);
    SetBoard(r2, c2, state);
    AddPiece(r2, c2, state);
  }
  history_.pop_back();
}

std::unique_ptr<State> BattleChessState::Clone() const {
  return std::unique_ptr<State>(new BattleChessState(*this));
}

BattleChessGame::BattleChessGame(const GameParameters& params)
    : Game(kGameType, params){}

int BattleChessGame::NumDistinctActions() const {
  return rows_ * cols_ * rows_ * cols_ * 9;
}

std::string BattleChessState::Serialize() const {
  std::string str = "";
  for (int r = 0; r < rows_; r++) {
    for (int c = 0; c < cols_; c++) {
      absl::StrAppend(&str, CellToString(board(r, c)));
    }
  }
  return str;
}

std::unique_ptr<State> BattleChessGame::DeserializeState(
    const std::string& str) const {
  std::unique_ptr<State> state = NewInitialState();

  if (str.length() != rows_ * cols_) {
    SpielFatalError("Incorrect number of characters in string.");
    return std::unique_ptr<State>();
  }

  BattleChessState* bstate = dynamic_cast<BattleChessState*>(state.get());

//   bstate->SetPieces(0, 0);
//   bstate->SetPieces(1, 0);
  // 分别对白棋和黑棋进行清空
  bstate->InitPieces(0);
  bstate->InitPieces(1);

  int i = 0;
  for (int r = 0; r < rows_; r++) {
    for (int c = 0; c < cols_; c++) {
      if (str.at(i) == 'k') {
        bstate->AddPiece(r, c, CellState::kBlackKing);
        bstate->SetBoard(r, c, CellState::kBlackKing);
      } else if (str.at(i) == 'd') {
        bstate->AddPiece(r, c, CellState::kBlackDefender);
        bstate->SetBoard(r, c, CellState::kBlackDefender);
      } else if (str.at(i) == 'a') {
        bstate->AddPiece(r, c, CellState::kBlackAttacker);
        bstate->SetBoard(r, c, CellState::kBlackAttacker);
      } else if (str.at(i) == 'K') {
        bstate->AddPiece(r, c, CellState::kWhiteKing);
        bstate->SetBoard(r, c, CellState::kWhiteKing);
      } else if (str.at(i) == 'D') {
        bstate->AddPiece(r, c, CellState::kWhiteDefender);
        bstate->SetBoard(r, c, CellState::kWhiteDefender);
      } else if (str.at(i) == 'A') {
        bstate->AddPiece(r, c, CellState::kWhiteAttacker);
        bstate->SetBoard(r, c, CellState::kWhiteAttacker);
      } else if (str.at(i) == '.') {
        bstate->SetBoard(r, c, CellState::kEmpty);
      } else {
        std::string error = "Invalid character in std::string: ";
        error += str.at(i);
        SpielFatalError(error);
        return std::unique_ptr<State>();
      }

      i++;
    }
  }

  return state;
}

}  // namespace battle_chess
}  // namespace open_spiel
