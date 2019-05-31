#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int RowsCount(FILE *readtsv) {

	// tsvの列数をカウント
	char ch;
	int  rows = 0;
	int  confirmation_rows = 0;

	while ((ch = fgetc(readtsv)) != '\n') {                // 1行だけ探索
		if (ch == '\t') rows++;                            // タブでカウント
	}

	// ファイルポインタの巻き戻し
	fseek(readtsv, 0, SEEK_SET);

	while ((ch = fgetc(readtsv)) != EOF) {                 // ファイルの最後まで探索
		if (ch == '\t') confirmation_rows++;
		if (ch == '\n') {
			if (confirmation_rows != rows) {               // 例外処理
					printf("列数が一致していません。\n");
					system("pause");
					exit(1);
			}
			confirmation_rows = 0;
		}
	}

	// ファイルポインタの巻き戻し
	fseek(readtsv, 0, SEEK_SET);
	return rows+1;
}

int MaxSizeColumn(FILE *readtsv) {

	// 1行あたりの最大文字数をカウント
	char ch;
	int  chlength = 0
	  ,  chmax    = 0;

	while ((ch = fgetc(readtsv)) != EOF) {                // ファイルの最後まで探索
		chlength++;
		if (ch == '\n') {
			if (chlength > chmax) {                       // 改行文字含めて最大値を出す
				chmax = chlength + 1;
			}
			chlength = 0;
		}
	}

	// ファイルポインタの巻き戻し
	fseek(readtsv, 0, SEEK_SET);

	return chmax;
}

char *strtokc(char *source_ch, char search_ch) {

	// ヌルデータを格納できるstrtokの改良版
	static char *string_p = 0;

	if (source_ch) string_p = source_ch;             // 2度目以降に"0"でスタティックから呼び出し
	else	source_ch = string_p;
	
	if (!source_ch) return(0);                       // 文字列が存在しない場合

	while (1) {
		if (!*string_p) {                            // 検索文字が終端まで行った場合
			string_p = 0;
			return(source_ch);
		}

		if (*string_p == search_ch) {                // 検索文字が見つかった場合
			*string_p++ = 0;
			return(source_ch);
		}

		string_p++;
	}
}


void Normalization(FILE *readtsv,FILE *writetsv,int maxsize, int rows) {

	// tsvからデータを読み取って第一正規化して書き出す
	// セルに含まれる値(":"で区切られる値)のことを"要素"と定義します

	char *line_str                                     // 1行を読み取る
	   , *elem[6][11]                                  // 最大列5, 最大要素10を格納
	   , *temp_p;                                      // 文字列分割用
	int   rowscount  = 0
	   ,  carrycount = 0                               // 桁上がりの管理
	   ,  semicolon_count = 0
	   ,  elemcount[6]
	   ,  savecount[6];                                // 要素の数を保存

	// 文字列のためのメモリを確保
	line_str = (char *)malloc(maxsize + 5);
	if (line_str == NULL) {
		printf("メモリが確保できません\n");
		exit(1);
	}

	// １行ずつ読み込み
	while (fgets(line_str, maxsize+1, readtsv) != NULL) {
		rowscount = 0;

		// 初回のタブでセル分割
		temp_p = strtokc(line_str, '\t');

		// 2回目以降のタブでセル分割
		while (temp_p) {
			if (strlen(temp_p) > 10001) {                // 例外処理
				printf("セルが1万文字を超えています。\n");
				system("pause");
				exit(1);
			}

			for (int i = 0; temp_p[i] != '\0'; i++)      // 要素の個数
				if (temp_p[i] == ':') semicolon_count++;
			if (semicolon_count >= 10) {                 // 例外処理
				printf("セルの要素数が11以上あります。n");
				system("pause");
				exit(1);
			}
			semicolon_count = 0;

			elem[rowscount][0] = temp_p;                 //配列にセルを格納
			temp_p = strtokc(0, '\t');
			rowscount++;
		}

		// ":"で要素を分割
		for (int i = 0; i < rows; i++) {
			elemcount[i] = 0;

			// 初回の":"で要素分割
			temp_p = strtokc(elem[i][elemcount[i]], ':');

			// 2回目以降の":"で要素分割
			while (temp_p) {
				elem[i][elemcount[i]] = temp_p;   //配列に要素を格納
				temp_p = strtokc(0, ':');
				elemcount[i]++;
			}

			// 末尾の改行外し
			elem[i][elemcount[i]-1] = strtokc(elem[i][elemcount[i] - 1], '\n');

			//各要素の個数を記録
			savecount[i] = elemcount[i];
		}

		//  ファイルにデータを書き込む
		while (1) {
			for (int i = 0; i < rows; i++) {
				if (i == rows - 1) fprintf(writetsv, "%s\n", elem[i][elemcount[i] - 1]);       // 末尾なら改行書き出し
				else fprintf(writetsv, "%s\t", elem[i][elemcount[i] - 1]);                     // それ以外でタブ書き出し
			}

			// 桁上がりの処理と添え字の管理
			carrycount = 0;
			for (int i = 0; i < rows; i++) {
				if (elemcount[i] == 1) {
					elemcount[i] = savecount[i];                     // 要素が一巡したら、初期値に戻して桁上げる
					carrycount++;
				}else{
					elemcount[i]--;                                  // 要素を１つずらしてブレーク
					break;
				}
			}

			//桁上がりの連続回数が列数と同じなら一巡した
			if (carrycount == rows) break;
		}
	}

	free(line_str);
	return;
}

void main() {
	FILE *readfile;                                         // ファイルポインタ（入力用）
	FILE *writefile;                                        // ファイルポインタ（出力用）
	int maxlinesize = 0;                                    // 読み込むファイルの最大長文字数
	int maxrow = 0;                                         // 最大列数
	
	// ファイルを開く
	readfile  = fopen("c:\\test\\sample.tsv", "r");
	writefile = fopen("c:\\test\\result.tsv", "w");

	if (readfile == NULL) {                                 // オープンに失敗した場合
		printf("ファイルが開rけません。\n");
		exit(1);
	}
	printf("ファイル読み込み完了。\n");

	maxlinesize = MaxSizeColumn(readfile);                  // 最大文字数カウント
	maxrow = RowsCount(readfile);                           // 列数カウント

	if (maxrow >= 6 || maxrow <= 0) {                       // 例外処理
		printf("列数が6以上,または0以下です。\n");
		system("pause");
		exit(1);
	}

	if (maxlinesize>=50007) {                               // 例外処理
		printf("1行あたりの最大文字数が50000を超えています。\n");
		system("pause");
		exit(1);
	}

	printf("正規化を開始します。\n");
	Normalization(readfile, writefile, maxlinesize, maxrow);// 正規化と書き出し
	printf("正規化が完了しました。\n");

	// ファイルを閉じる
	fclose(readfile);
	fclose(writefile);

	system("pause");                                        // 何か入力を受け取って終了
}