# Image Recoder & Analyzer for Kiribako & Iontrap

Small program to record and analyze images/videos captured by USB webcam or microscope via OpenCV.
It is used for Kiribako (cloud chamber) and Iontrap experiments.
It runs fine at least on Debian GNU/Linux.

## 実行ファイル

実行ファイルの種類と用途は実験ウェブページで説明されています。
以下の手順でビルドできます: 
```
cd kiribako/src
make install
```

古の Makefile 直書きなので、別環境で動く保証は有りません。

## 基本的なファイル操作の手順

実験で記録した画像・動画ファイルの扱い方について。
既にコンピューターが適切に設定されているとして、テキスト端末から以下の手順で操作する。

### 自動アップロード機能

対象ディレクトリ (`export`) 内のファイルは user crontab により10分毎にサーバーへアップロードされる。
実験期間外やテスト中にアップロードを無効化したい場合は、以下のコマンドを実行する (どのディレクトリからでも実行可): 
```
cd kiribako
./bin/sync-export.sh off
```

有効化したい場合:
```
./bin/sync-export.sh on
```

また、アップロードはいつでも手動で実行できる:
```
./bin/sync-export.sh
```

### 霧箱実験 (放射線計測1・2) 用の手順

実験で記録するファイル (`kiribako-*.png`) が多いので、ファイルを自動的にサブディレクトリへ振り分けるスクリプトが用意してある。
(1日の実験が終わったら不要なファイルを消してから) 以下のコマンドを実行する: 
```
cd kiribako
./bin/move-kiribako-png.sh
./bin/sync-export.sh
```

ファイルが `export` 以下のサブディレクトリへ移され、サブディレクトリ毎の ZIP ファイルも作られる。

### イオントラップ実験用の手順

アップロードしたいファイル (=学生が欲しいファイル) を手動で `export` へ置く。

最初にラウンド毎のディレクトリを作っておく: 
```
cd kiribako/export
mkdir round_{1,2,3}
```

動画ファイル (`iontrap-*.avi` ) とフレーム画像ファイル (`frame_*.png`) は、(不要なファイルは消してから) 以下の手順でアップロードできる: 
```
cd kiribako
mv frame_*.png iontrap-*.avi   export/round_1
./bin/sync-export.sh
```

また、4日目の比電荷の計算で使った `calc_iontrap_eom_day4.xlsx` 等もアップロードします:
```
cd
mv Documents/calc_iontrap_eom_day4.xlsx kiribako/export/round_1
```
