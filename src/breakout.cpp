#include <opencv2/opencv.hpp>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

//*** 各種パラメータ ***//
static const int BALL_RADIUS = 3;
static const int PLAYER_WIDTH = 40;
static const int PLAYER_HEIGHT = 6;
static const int PLAYER_MOVE = 2;
static const int BLOCK_RADIUS = 15;
static const int BLOCK_NUM = 90;
static const int BLOCK_START_POINT_X = 28;
static const int BLOCK_START_POINT_Y = 50;
static const int BLOC_EVERY_LINE_NUM  = 18;

//ゲームオーバーフラグ
bool gameover;
bool gameclear;


//*** キー入力受け付け関数 ***//
int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if (ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

//*** 各オブジェクトの親クラス ***//
class Object
{
public:
  int width;      //オブジェクトの幅
  int height;     //オブジェクトの高さ
  cv::Point pos;  //ディスプレイ中のオブジェクトの位置
  
  //初期位置をコンストラクタで決定
  Object(int x, int y){
    this->pos = cv::Point(x,y);
  }
  
  //描画関数
  void draw(cv::Mat& screen){
    for(int y = this->pos.y - this->height/2; y < this->pos.y + this->height/2; y++)
      for(int x = this->pos.x - this->width/2; x < this->pos.x + this->width/2; x++)
  	screen.at<unsigned char>(y,x) = 255;
  }
  
};

//*** プレイヤークラス ***//
class Player : public Object
{
  enum CONTROL {LEFT, STOP, RIGHT};   //プレイヤーのステート
public:
  CONTROL ctrl = STOP;   //ステートを保持する変数

  //コンストラクタで初期位置を設定
  Player(int x, int y) : Object(x,y)
  {
    this->width = PLAYER_WIDTH;
    this->height = PLAYER_HEIGHT;
  }
  
  //キー入力からステート変更
  void key_input(){
    if(kbhit()){
      switch(getchar()){
      case 'b': this->ctrl=LEFT; break;
      case 'n': this->ctrl=STOP; break;
      case 'm': this->ctrl=RIGHT; break;
      default: break;
      }
    }
  }
  
  //ステートによって挙動が変わる
  void control(const cv::Mat& src){
    switch(this->ctrl){
    case LEFT:  if(this->pos.x-this->width/2-PLAYER_MOVE >= 0) this->pos.x-=PLAYER_MOVE; break;
    case STOP:  break;
    case RIGHT: if(this->pos.x+this->width/2+PLAYER_MOVE <= src.cols) this->pos.x+=PLAYER_MOVE; break;
    default: break;
    }
  }
};

//*** ブロッククラス ***//
class Block : public Object
{
private:
  bool exist;   //ブロックが存在しているかどうかを表す変数

public:
  static int non_exist_num; //消えたブロックの数

  //コンストラクタでブロックの初期位置を決定
  Block(int x, int y) : Object(x,y)
  {
    this->width = 2*BLOCK_RADIUS;
    this->height = 2*BLOCK_RADIUS;
    exist = true;
  }
  
  //ボールとブロックが衝突したときにブロックの存在を消す関数
  void not_exist(){
    this->exist = false;
    non_exist_num++;
  }
  
  bool isExisting(){
    return this->exist;
  }
  
};

int Block::non_exist_num;

//*** ボールクラス ***//
class Ball : public Object
{
public:
  bool vec_x; //x方向のベクトル 正 1 : 負 0
  bool vec_y; //y方向のベクトル 正 1 : 負 0
  
  //コンストラクタでボールの初期位置を決定
  Ball(int x, int y) : Object(x,y)
  {
    this->width = 2*BALL_RADIUS;
    this->height = 2*BALL_RADIUS;
    vec_x = 1;
    vec_y = 1;
  }
  
  //ボールとプレイヤーの衝突判定
  bool colision_with_player(const cv::Mat& src, Player player){
    if(abs(this->pos.x-player.pos.x) <= this->width/2+player.width/2 && abs(this->pos.y-player.pos.y) <= this->height/2+player.height/2)
      return true;
    return false;
  }
  
  //ボールとブロックの衝突判定
  bool colision_with_block(const cv::Mat& src, Block block){
    if(abs(this->pos.x-block.pos.x) <= this->width/2+block.width/2 && abs(this->pos.y-block.pos.y) <= this->height/2+block.height/2)
      return true;
    return false;
  }
  
  //衝突に従ってボールのベクトルを変更
  void update_vec(const cv::Mat& src, std::vector<Block>& block){
    //object
    for(int y = this->pos.y - this->height/2; y < this->pos.y + this->height/2; y++){
      if(src.at<unsigned char>(y,this->pos.x+this->width/2) == 255){
    	this->vec_x = 0;
	for(int i = 0; i < block.size(); i++)
	  if(colision_with_block(src,block[i]))
	    block[i].not_exist();
      }
    }
    for(int y = this->pos.y - this->height/2; y < this->pos.y + this->height/2; y++){
      if(src.at<unsigned char>(y,this->pos.x-this->width/2-1) == 255){
    	this->vec_x = 1;
	for(int i = 0; i < block.size(); i++)
	  if(colision_with_block(src,block[i]))
	    block[i].not_exist();
      }
    }
    for(int x = this->pos.x - this->height/2; x < this->pos.x + this->height/2; x++){
      if(src.at<unsigned char>(this->pos.y-this->width/2-1,x) == 255){
    	this->vec_y = 0;
	for(int i = 0; i < block.size(); i++)
	  if(colision_with_block(src,block[i]))
	    block[i].not_exist();
      }
    }
    for(int x = this->pos.x - this->height/2; x < this->pos.x + this->height/2; x++){
      if(src.at<unsigned char>(this->pos.y+this->width/2,x) == 255){
    	this->vec_y = 1;
	for(int i = 0; i < block.size(); i++)
	  if(colision_with_block(src,block[i]))
	    block[i].not_exist();
      }
    }

    //壁の衝突判定
    //wall
    if(this->pos.x+this->width/2 >= src.cols)
      this->vec_x = 0;
    if(this->pos.x-this->width/2 <= 0)
      this->vec_x = 1;
    if(this->pos.y-this->height/2 <= 0)
      this->vec_y = 0;
    
    //下部に衝突したらゲームオーバー
    if(this->pos.y+this->height/2 >= src.rows)
      gameover = true;

  }
  //ベクトルに従ってボールの位置を更新
  void update_pos(){
    if(this->vec_x)
      this->pos.x++;
    else
      this->pos.x--;
    if(this->vec_y)
      this->pos.y--;
    else
      this->pos.y++;
  }
};

//すべてのブロックが消えたらゲームクリア
bool clear_check(){
  if(Block::non_exist_num >= BLOCK_NUM)
    gameclear = true;
}

//*** メイン関数 ***//
int main(int argc,char** argv)
{
  //*** 初期化 ***//
  //ディスプレイ生成
  cv::Mat screen = cv::Mat(cv::Size(600,400),CV_8UC1);
  
  //ボール生成
  Ball ball(screen.cols/2,2*screen.rows/3);
  
  //プレイヤー生成
  Player player(screen.cols/2,7*screen.rows/8);
  
  //ブロック生成
  std::vector<Block> block;
  for(int i = 0; i < BLOCK_NUM; i++){
    Block b(BLOCK_START_POINT_X + (i%BLOC_EVERY_LINE_NUM)*(BLOCK_RADIUS+1)*2,
	    BLOCK_START_POINT_Y + (i/BLOC_EVERY_LINE_NUM)*(BLOCK_RADIUS+1)*2);
    block.push_back(b);
  }
  //消えたブロックの数を初期化
  Block::non_exist_num = 0;
  
  //操作説明メッセージ
  std::cout << "#########CONTROLS################" << std::endl;
  std::cout << "#                               #" << std::endl;
  std::cout << "# move to the right : press 'm' #" << std::endl;
  std::cout << "# stop the moving   : press 'n' #" << std::endl;
  std::cout << "# move to the left  : press 'b' #" << std::endl;
  std::cout << "#                               #" << std::endl;
  std::cout << "#################################" << std::endl;
  std::cout << std::endl << "press 's' to start the game!" << std::endl; 

  //*** 初期画面 ***//
  while(1){
    /* 画面更新 */
    screen = cv::Scalar::all(0);
    
    /* 描画 */
    //ball
    ball.draw(screen);
    
    //player
    player.draw(screen);
    
    //block
    for(int i = 0; i < block.size(); i++)
      block[i].draw(screen);

    /* 表示 */
    cv::imshow("screen",screen);
    cv::waitKey(4);

    //'s'キーが入力されたらゲームスタート
    if(kbhit())
      if(getchar() == 's')
  	break;
  }

  //*** ゲーム開始メッセージ ***//
  std::cout << std::endl;
  std::cout << "#############" << std::endl;
  std::cout << "#GAME START!#" << std::endl;
  std::cout << "#############" << std::endl << std::endl;

  //*** ゲーム画面を更新し続ける ***//
  while(1){
    /* 画面更新 */
    screen = cv::Scalar::all(0);
    
    /* 描画 */
    //ball
    ball.draw(screen);
    
    //player
    player.draw(screen);
    
    //block
    for(int i = 0; i < block.size(); i++){
      //消えてないブロックを描画
      if(block[i].isExisting()){
	block[i].draw(screen);
      }
    }
    
    /* 表示 */
    cv::imshow("screen", screen);
    cv::waitKey(4);
    
    /* 値更新 */
    clear_check(); //クリアしたかどうかをチェック
    
    //ball
    ball.update_vec(screen,block); //ボールのベクトルを更新
    ball.update_pos(); //ボールの位置を更新
    
    //player
    player.key_input(); //キー入力の受け付け
    player.control(screen); //プレイヤーの位置を更新

    //ループを続けるかどうかの判定
    if(gameover | gameclear)
      break;
  }

  //ゲームオーバーのとき
  if(gameover){
    //*** ゲームオーバーメッセージ ***//
    std::cout << std::endl << std::endl;
    std::cout << "############" << std::endl;
    std::cout << "#GAME OVER!#" << std::endl;
    std::cout << "############" << std::endl;
  }
  //ゲームクリアのとき
  else if(gameclear){
    //*** ゲームクリアメッセージ ***//
    std::cout << std::endl << std::endl;
    std::cout << "☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆" << std::endl;
    std::cout << "☆             ☆" << std::endl;
    std::cout << "☆ GAME CLEAR! ☆" << std::endl;
    std::cout << "☆             ☆" << std::endl;
    std::cout << "☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆" << std::endl;
  }
  return 0;
}
