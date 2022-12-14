# EV充電コンセントを、Nature RemoにEVPSと誤認させて遠隔制御する

この記事は、[Nature Remo Advent Calendar 2022 17日目](https://adventar.org/calendars/7975)の記事として記載したものです。

エンジニアブログの方に書いてもよかったのですが、まだまだ内容が杜撰なため締め切りに間に合うためにGitHub Pages上にて公開しています。

## 自然な導入
ところで、皆さんはEVに乗っていますか？Natureでも[EV作りを行っていた](https://engineering.nature.global/entry/blog-fes-2022-i-love-creo)ようですが、実用化に至るにはまだまだ課題があるようです。

そんなEVの普及と切っても切り離せないのが、充電器です。Natureではアプリケーション面では[EVPSの遠隔制御に対応](https://nature.global/press/release/7681/)するなどして、エネルギーの効率利用を促しています。

そんな我が家にも、EV充電器（EV用コンセント）が備え付けられています。

（~~写真を追加する~~残ってませんでした。）

しかし、EVの充電時は30Aと大きな電流を必要とし、特に夏冬シーズンはエアコンの利用も重なるとブレーカーが落ちる主な要因になります。
かといって、充電を開始するのは車で家に帰ってきた直後、プラグを挿した瞬間です。今まさに家の家電を付け始めるその直前に、EVの充電という大電流を先に消費し始めるのです。With EVライフの構造的欠陥が見え隠れするようです。

そこで、このコンセントを遠隔制御できるようにし、かつNature Remoアプリでそれを実行できるとBe Coolということで実装します。

## システム構成
（気が向いたら図を入れます）

## M5Stack側の準備
M5Stackには、ECHONET Liteプロトコルを実装しまるでEVPSが家にあるかのように振舞います。
Nature Remo Eが家に設置されていれば、「エコネットライト機器を追加」することで、このM5Stackに機器オブジェクトに対応したプロパティの送受信が可能になります。

### ECHONET Liteの説明
気が向いたら書きます。

### 充電・待機コマンドに応じて充電コンセントを操作する
我が家の充電コンセントはスイッチ付きのタイプです。また、いたずら防止のために簡易ロックのほか、南京錠などを追加で設置することも可能です。
（~~写真を追加する~~残ってませんでした。）

スイッチを操作すると言ったら、そうSwitchBot Botですね。**＜ゃι ﾚヽ**のでリンクは貼りません。
この[BotをM5Stackから操作するリポジトリ](https://github.com/ixsiid/switchbot-nimble-client)が都合よくあったのでこちらを使えばよさそうです。

Nature RemoアプリからECHONET Liteプロトコルを通して、EVPSオブジェクトの充電コマンドや待機コマンドに応じてSwitchBot Botを動かす。
全体像が見えてきたので、[実装しま ~~す。~~ した。](https://github.com/ixsiid/m5stack-echonet-evps)

## 屋外設置
こちらのコンセント付属のスイッチ、屋外しようということで防水カバーもついており、ストロークも長めのプッシュ-プッシュスイッチになっています。
Botをここに設置するために、防水カバーの新設が必要になります。
既存のカバーを切り抜いて、[Botが収まるカバーを別に製作](https://github.com/ixsiid/m5stack-echonet-evps/releases/tag/v0.0.1)し覆うように設置しました。

<img
 src="https://user-images.githubusercontent.com/35385895/208041078-f824fefc-06e3-4543-bd30-6275e6d588b6.jpg" width="20%" /><img
 src="https://user-images.githubusercontent.com/35385895/208041065-5d6a5a9a-f866-41e6-8724-5e033057ca4a.jpg" width="20%" /><img 
 src="https://user-images.githubusercontent.com/35385895/208041076-a0c59897-89f9-4679-a5ab-0a481b60bff5.jpg" width="20%" />

### 対候性の工夫
1. 3Dプリント品の上にUVレジンを一度塗布、硬化後に表面を削って滑らかにしています。
2. 固定にはポリカ性のネジを使用しました。錆で汚れると悲しくなるので。
3. 設置後の隙間にはシリコン充填しています。

## 動作テスト
### EVPS ON
[![EVPS ONの動作映像](http://img.youtube.com/vi/djXRR8WapNw/0.jpg)](https://www.youtube.com/watch?v=djXRR8WapNw)
### EVPS OFF
[![EVPS OFFの動作映像](http://img.youtube.com/vi/P2LOBnNnHTk/0.jpg)](https://www.youtube.com/watch?v=P2LOBnNnHTk)


EVPSの操作に応じて、瞬時電力が変わっているのがわかると思います。
操作から充電までにタイムラグがあるように見えますが、瞬時電力の反映に時間がかかるようです。

## おわりに
とりあえず、Nature Remoアプリから動かすという最初の目標はクリアすることが出来ました。
Botの操作に失敗するときがある。その時にアプリ上の表記と同期しなくなる。アプリのフロー図に反映されないなどの課題がありますので、プルリクお待ちしています。
