#include "bitboard.h"
#include <algorithm>
#include <cstring>
#include <random>
#include "types.h"

magic_bb magics[num_squares][2];
bitboard king_shield_magics[num_colors][num_squares];
bitboard get_align_mask[num_squares][num_squares];
bitboard get_anti_diag[num_squares];
bitboard get_check_ray[num_squares][num_squares];
bitboard get_double_check[num_squares];
bitboard get_file_bb[num_squares];
bitboard get_king_attacks[num_squares];
bitboard get_king_shield[num_colors][num_squares];
bitboard get_knight_attacks[num_squares];
bitboard get_main_diag[num_squares];
bitboard get_passers[num_colors][4][1 << 10];
bitboard get_pawn_attacks[num_colors][num_squares];
bitboard pext_table[0x1a480];

int get_king_shield_scores[num_colors][num_squares][1 << 6];
u8 get_castle_masks[num_colors][1 << 5];
u8 get_center_dist[num_squares];
u8 get_dist[num_squares][num_squares];

inline bitboard rand64(){
  static std::mt19937_64 gen(std::random_device{}());
  static std::uniform_int_distribution<bitboard> dis;
  return dis(gen);
}

namespace{
  bitboard pdep(bitboard mask,int src){
    bitboard dst=0;
    for(;src;clear_lsb(mask),src>>=1) if(src&1) dst|=lsb_mask(mask);
    return dst;
  }

  bitboard attacks_bb(const piece_type pt,const square sq,const bitboard occupied){
    bitboard attacks=0;
    dir rook_directions[4]={north,east,south,west};
    for(dir bishop_directions[4]={northeast,southeast,southwest,northwest};const dir d:pt==bishop?bishop_directions:rook_directions){
      square s=sq;
      do{
        attacks|=safe_step(s,d);
      } while(safe_step(s,d)&&!(square_bb(s+=d)&occupied));
    }
    return attacks;
  }

  bitboard sparse_rand64(std::mt19937_64& rng,const int count=3){
    bitboard result=~0ULL;
    for(int i=0;i<count;++i) result&=rng();
    return result;
  }

  void init_magics(){
    bitboard magic,* base=pext_table;
    std::vector<bitboard> occupied(4096);
    std::vector<bitboard> attacks(4096);
    for(const piece_type pt:{bishop,rook})
      for(square sq=h1;sq<=a8;sq++){
        const bitboard edge_files=(file_abb|file_hbb)&~file_bb(sq);
        const bitboard edge_ranks=(rank_1_bb|rank_8_bb)&~rank_bb(sq);
        const bitboard mask=attacks_bb(pt,sq,0)&~(edge_files|edge_ranks);
        magic_bb& m=magics[sq][pt-bishop];
        const int permutations=1<<popcnt(mask);
        bool failed=false;
        m.shift=64-popcnt(mask);
        m.ptr=base;
        m.mask=mask;
        for(int p=0;p<permutations;p++){
          occupied[p]=pdep(mask,p);
          attacks[p]=attacks_bb(pt,sq,occupied[p]);
        }
        std::mt19937_64 rng(rand64());
        do{
          magic=sparse_rand64(rng);
          memset(base,0,sizeof(bitboard)*permutations);
          for(int p=0,key=0;p<permutations;key=SC<int>(occupied[++p]*magic>>m.shift)){
            if((failed=base[key]&&base[key]!=attacks[p])) break;
            base[key]=attacks[p];
          }
        } while(failed);
        m.magic=magic;
        base+=permutations;
      }
  }
}

void bitboards::init(){
  for(square s1=h1;s1<=a8;s1++){
    get_file_bb[s1]=file_hbb<<s1%8;
    for(square s2=h1;s2<=a8;s2++) get_dist[s1][s2]=SC<u8>(std::max(file_dist(s1,s2),rank_dist(s1,s2)));
  }
  init_magics();
  for(square s1=h1;s1<=a8;s1++){
    get_main_diag[s1]=(bishop_attacks(s1,0)&(mask(s1,northwest)|mask(s1,southeast)))|square_bb(s1);
    get_anti_diag[s1]=(bishop_attacks(s1,0)&(mask(s1,northeast)|mask(s1,southwest)))|square_bb(s1);
    get_center_dist[s1]=SC<u8>(std::min({
    rank_dist(s1,e4)+file_dist(s1,e4),
    rank_dist(s1,e5)+file_dist(s1,e5),
    rank_dist(s1,d4)+file_dist(s1,d4),
    rank_dist(s1,d5)+file_dist(s1,d5)
    }));
    for(square s2=h1;s2<=a8;s2++)
      if(piece_type pt;attacks_bb(pt=bishop,s1,0)&square_bb(s2)||attacks_bb(pt=rook,s1,0)&square_bb(s2)){
        get_check_ray[s1][s2]=(attacks_bb(pt,s1,square_bb(s2))&attacks_bb(pt,s2,square_bb(s1)))|square_bb(s2);
        get_align_mask[s1][s2]=(attacks_bb(pt,s1,0)&attacks_bb(pt,s2,0))|square_bb(s1,s2);
      }
    for(const dir d:{north,northeast,east,southeast,south,southwest,west,northwest}) get_king_attacks[s1]|=safe_step(s1,d);
    for(const dir d:{north+northeast,northeast+east,southeast+east,south+southeast,
        south+southwest,southwest+west,northwest+west,north+northwest
        })
      get_knight_attacks[s1]|=safe_step(s1,d);
    const square sq=get_square(get_rank(s1),file_g);
    get_king_shield[white][s1]=
      ((rank_bb(sq+north)|rank_bb(sq+north+north))&~mask(sq+west,west))<<std::clamp(s1%8-1,0,5)&mask(s1,north);
    get_king_shield[black][s1]=
      ((rank_bb(sq+south)|rank_bb(sq+south+south))&~mask(sq+west,west))<<std::clamp(s1%8-1,0,5)&mask(s1,south);
    get_double_check[s1]=get_king_attacks[s1]|get_knight_attacks[s1];
    get_pawn_attacks[white][s1]=pawn_attacks<white>(square_bb(s1));
    get_pawn_attacks[black][s1]=pawn_attacks<black>(square_bb(s1));
  }
  for(const color c:{white,black}){
    const bitboard relevancy=rel_rank(c,rank_3,rank_4,rank_5,rank_6,rank_7);
    const bitboard masks[4]={relevancy&rel_file(c,file_g,file_h),
    relevancy&rel_file(c,file_e,file_f),
    relevancy&rel_file(c,file_c,file_d),
    relevancy&rel_file(c,file_a,file_b),
    };
    for(int i=0;i<4;i++){
      for(int j=0;j<1<<popcnt(masks[i]);j++){
        bitboard spans=0;
        for(bitboard thempawn=pdep(masks[i],j);thempawn;clear_lsb(thempawn)){
          const square s=lsb(thempawn);
          spans|=(file_bb(s)|(file_bb(s+east)&~file_abb)|(file_bb(s+west)&~file_hbb))&mask(s,rel_direction(c,south));
        }
        get_passers[c][i][j]=~spans;
      }
    }
  }
  for(int i=0;i<1<<5;i++){
    constexpr u8 clear_k=static_cast<u8>(~8);
    constexpr u8 clear_q=static_cast<u8>(~4);
    constexpr u8 cleark=static_cast<u8>(~2);
    constexpr u8 clearq=static_cast<u8>(~1);
    const bitboard w_occ=pdep(square_bb(a1,e1,h1,a8,h8),i);
    const bitboard b_occ=pdep(square_bb(a8,e8,h8,a1,h1),i);
    u8 w_mask=0xf;
    u8 b_mask=0xf;
    if((w_occ&square_bb(a1))==0) w_mask&=clear_q;
    if((w_occ&square_bb(e1))==0) w_mask&=clear_k&clear_q;
    if((w_occ&square_bb(h1))==0) w_mask&=clear_k;
    if((w_occ&square_bb(a8))!=0) w_mask&=clearq;
    if((w_occ&square_bb(h8))!=0) w_mask&=cleark;
    if((b_occ&square_bb(a8))==0) b_mask&=clearq;
    if((b_occ&square_bb(e8))==0) b_mask&=cleark&clearq;
    if((b_occ&square_bb(h8))==0) b_mask&=cleark;
    if((b_occ&square_bb(a1))!=0) b_mask&=clear_q;
    if((b_occ&square_bb(h1))!=0) b_mask&=clear_k;
    get_castle_masks[white][w_occ*0x4860104020003061ull>>59]=w_mask;
    get_castle_masks[black][b_occ*0x1080000400400c21ull>>59]=b_mask;
  }
  for(const color c:{white,black})
    for(square ksq=h1;ksq<=a8;ksq++){
      const bitboard mask=get_king_shield[c][ksq];
      bitboard occupied[64],magic;
      const int permutations=1 << popcnt(mask);
      std::vector<bool> visited(permutations);
      bool failed=false;
      for(int p=0;p<permutations;p++) occupied[p]=pdep(mask,p);
      std::mt19937_64 rng(rand64());
      do{
        magic=sparse_rand64(rng);
        for(int p=0,key=0;p<permutations;key=SC<int>(occupied[++p]*magic>>(64-popcnt(mask)))){
          if((failed=visited[key])) break;
          visited[key]=true;
        }
        std::fill(visited.begin(), visited.end(), false);
      } while(failed);
      king_shield_magics[c][ksq]=magic;
      for(int i=0;i<1<<popcnt(get_king_shield[c][ksq]);i++){
        const bitboard king_shield=pdep(get_king_shield[c][ksq],i);
        const int hash=SC<int>(king_shield*king_shield_magics[c][ksq]>>58);
        constexpr int min_score=-45,max_score=45;
        if(!(square_bb(ksq)&(c==white?rank_1_bb:rank_8_bb))||popcnt(king_shield)<2){
          get_king_shield_scores[c][ksq][hash]=min_score;
          continue;
        }
        if(square_bb(ksq)&(file_ebb|file_dbb)){
          get_king_shield_scores[c][ksq][hash]=10;
          continue;
        }
        const int file_weights[8][3]=
        {
        {40,50,45},{40,50,45},
        {50,45,40},{0,0,0},
        {0,0,0},{40,45,50},
        {45,50,40},{45,50,40}
        };
        bitboard shield_mask=get_king_shield[c][ksq];
        const bitboard file_right=file_bb(lsb(shield_mask));
        const bitboard file_mid=file_right<<1;
        const bitboard file_left=file_right<<2;
        int value=0;
        const int king_file=ksq&7;
        if((king_shield&file_right)==0) value-=file_weights[king_file][0];
        if((king_shield&file_mid)==0) value-=file_weights[king_file][1];
        if((king_shield&file_left)==0) value-=file_weights[king_file][2];
        for(int j=0;shield_mask;clear_lsb(shield_mask),j++){
          const int pawn_weights[8][6]=
          {
          {10,20,15,5,10,5},{5,25,15,0,0,5},
          {20,15,10,0,0,0},{0,0,0,0,0,0},
          {0,0,0,0,0,0},{10,15,20,0,0,0},
          {15,25,5,5,0,0},{15,20,10,5,10,5}
          };
          const int index=c==white?j:j<3?j+3:j-3;
          if(king_shield&square_bb(lsb(shield_mask))) value+=pawn_weights[king_file][index];
        }
        get_king_shield_scores[c][ksq][hash]=std::clamp(value,min_score,max_score);
      }
    }
}

std::string to_string(const bitboard b){
  const std::string l="+---+---+---+---+---+---+---+---+\n";
  std::string s=l;
  for(bitboard bit=square_bb(a8);bit;bit>>=1){
    s+=bit&b?"| @ ":"|   ";
    if(bit&file_hbb) s+="|\n"+l;
  }
  return s+"\n";
}
