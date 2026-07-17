// WiFi 配网页面 HTML — 支持 Wi-Fi 扫描和城市名本地搜索；不回显设备已保存的凭据。
const char CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
  <title>Weather Clock — Setup</title>
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

    :root {
      --bg:        #0d0f14;
      --surface:   #161a22;
      --border:    #252c3b;
      --accent:    #00e5c9;
      --accent2:   #4a8cff;
      --warn:      #ff6b35;
      --text:      #c8d0df;
      --muted:     #5a6478;
      --success-bg:#0a2e28;
      --error-bg:  #2e1208;
      --mono:      ui-monospace, SFMono-Regular, Consolas, monospace;
      --sans:      system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
    }

    body {
      background: var(--bg);
      color: var(--text);
      font-family: var(--sans);
      font-weight: 300;
      min-height: 100vh;
      display: flex;
      align-items: flex-start;
      justify-content: center;
      padding: 24px 16px 48px;
    }

    .container { width: 100%; max-width: 420px; }

    .header {
      text-align: center;
      padding: 32px 0 28px;
    }
    .header-icon {
      width: 56px; height: 56px;
      margin: 0 auto 16px;
      background: linear-gradient(135deg, var(--accent), var(--accent2));
      border-radius: 16px;
      display: flex; align-items: center; justify-content: center;
      font-size: 26px;
      box-shadow: 0 0 24px rgba(0,229,201,.25);
    }
    .header h1 {
      font-family: var(--mono);
      font-size: 1.1rem;
      letter-spacing: 0.18em;
      color: #fff;
      text-transform: uppercase;
    }
    .header p {
      font-size: 0.78rem;
      color: var(--muted);
      margin-top: 6px;
      letter-spacing: 0.04em;
    }

    .card {
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: 16px;
      padding: 28px 24px;
      position: relative;
      overflow: hidden;
    }
    .card::before {
      content: '';
      position: absolute;
      top: 0; left: 0; right: 0;
      height: 2px;
      background: linear-gradient(90deg, var(--accent), var(--accent2), transparent);
    }

    .section-label {
      font-family: var(--mono);
      font-size: 0.68rem;
      letter-spacing: 0.12em;
      text-transform: uppercase;
      color: var(--accent);
      margin-bottom: 16px;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .section-label::after {
      content: '';
      flex: 1;
      height: 1px;
      background: var(--border);
    }

    .section + .section { margin-top: 28px; }

    .field { margin-bottom: 14px; }
    .field label {
      display: block;
      font-size: 0.72rem;
      color: var(--muted);
      margin-bottom: 6px;
      letter-spacing: 0.06em;
    }
    .input-wrap { position: relative; }
    .input-wrap input {
      width: 100%;
      background: var(--bg);
      border: 1px solid var(--border);
      border-radius: 8px;
      color: var(--text);
      font-family: var(--mono);
      font-size: 0.9rem;
      padding: 10px 14px;
      outline: none;
      transition: border-color .2s, box-shadow .2s;
      -webkit-appearance: none;
    }
    .input-wrap input:focus {
      border-color: var(--accent);
      box-shadow: 0 0 0 3px rgba(0,229,201,.1);
    }
    .input-wrap input::placeholder { color: var(--muted); opacity: .6; }
    .eye-btn {
      position: absolute; right: 10px; top: 50%; transform: translateY(-50%);
      background: none; border: none; cursor: pointer;
      color: var(--muted); padding: 4px; line-height: 1;
      transition: color .2s;
    }
    .eye-btn:hover { color: var(--text); }

    /* ── WiFi 扫描行 ── */
    .scan-row {
      display: flex; align-items: flex-end; gap: 10px;
      margin-bottom: 14px;
    }
    .scan-row .field { flex: 1; margin-bottom: 0; }
    .icon-btn {
      flex-shrink: 0;
      background: none;
      border: 1px solid var(--border);
      border-radius: 8px;
      color: var(--accent);
      font-family: var(--mono);
      font-size: 0.72rem;
      letter-spacing: 0.08em;
      padding: 10px 14px;
      cursor: pointer;
      transition: background .2s, border-color .2s;
      display: flex; align-items: center; gap: 6px;
      white-space: nowrap;
    }
    .icon-btn:hover { background: rgba(0,229,201,.06); border-color: var(--accent); }
    .icon-btn.spinning .spin-icon { animation: spin .8s linear infinite; }
    @keyframes spin { to { transform: rotate(360deg); } }

    /* ── 下拉列表（WiFi & 城市共用） ── */
    .drop-list {
      background: var(--bg);
      border: 1px solid var(--border);
      border-radius: 8px;
      max-height: 200px;
      overflow-y: auto;
      display: none;
      margin-bottom: 14px;
    }
    .drop-list::-webkit-scrollbar { width: 4px; }
    .drop-list::-webkit-scrollbar-track { background: transparent; }
    .drop-list::-webkit-scrollbar-thumb { background: var(--border); border-radius: 4px; }
    .drop-list.open { display: block; }

    .drop-item {
      display: flex; align-items: center;
      padding: 10px 14px;
      cursor: pointer;
      border-bottom: 1px solid var(--border);
      transition: background .15s;
      gap: 10px;
    }
    .drop-item:last-child { border-bottom: none; }
    .drop-item:hover { background: rgba(255,255,255,.04); }
    .drop-item.selected { background: rgba(0,229,201,.07); }

    .item-main {
      flex: 1;
      font-size: 0.85rem;
      color: var(--text);
      white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
    }
    .item-sub {
      font-size: .7rem;
      color: var(--muted);
      white-space: nowrap;
    }
    .item-meta {
      display: flex; align-items: center; gap: 6px;
      flex-shrink: 0;
    }
    .lock-icon { color: var(--warn); font-size: 0.7rem; }
    .sig { display: flex; align-items: flex-end; gap: 2px; height: 14px; }
    .sig span { display: block; width: 3px; background: var(--muted); border-radius: 1px; }
    .sig span:nth-child(1) { height: 4px; }
    .sig span:nth-child(2) { height: 7px; }
    .sig span:nth-child(3) { height: 10px; }
    .sig span:nth-child(4) { height: 14px; }
    .sig.s4 span { background: var(--accent); }
    .sig.s3 span:nth-child(1),
    .sig.s3 span:nth-child(2),
    .sig.s3 span:nth-child(3) { background: var(--accent); }
    .sig.s2 span:nth-child(1),
    .sig.s2 span:nth-child(2) { background: var(--accent2); }
    .sig.s1 span:nth-child(1) { background: var(--warn); }
    .rssi-label {
      font-size: .65rem;
      color: var(--muted);
      font-family: var(--mono);
    }

    /* ── 城市选中徽章 ── */
    .city-badge {
      display: none;
      align-items: center;
      gap: 8px;
      margin-top: 8px;
      padding: 8px 12px;
      background: rgba(0,229,201,.08);
      border: 1px solid rgba(0,229,201,.25);
      border-radius: 8px;
      font-size: 0.78rem;
      color: var(--accent);
      font-family: var(--mono);
    }
    .city-badge.show { display: flex; }
    .city-badge .clear-btn {
      margin-left: auto;
      background: none;
      border: none;
      color: var(--muted);
      cursor: pointer;
      font-size: 1rem;
      line-height: 1;
      padding: 0 2px;
      transition: color .2s;
    }
    .city-badge .clear-btn:hover { color: var(--warn); }

    .hint {
      font-size: 0.7rem;
      color: var(--muted);
      margin-top: -8px;
      margin-bottom: 14px;
      line-height: 1.5;
    }
    .hint a { color: var(--accent2); text-decoration: none; }

    .submit-btn {
      width: 100%;
      padding: 13px;
      background: linear-gradient(135deg, var(--accent), var(--accent2));
      color: #0d0f14;
      font-family: var(--mono);
      font-size: 0.85rem;
      font-weight: 700;
      letter-spacing: 0.1em;
      text-transform: uppercase;
      border: none;
      border-radius: 10px;
      cursor: pointer;
      transition: opacity .2s, box-shadow .2s;
      margin-top: 8px;
    }
    .submit-btn:hover { opacity: .9; box-shadow: 0 4px 20px rgba(0,229,201,.3); }
    .submit-btn:active { opacity: .8; }
    .submit-btn:disabled { opacity: .4; cursor: not-allowed; }

    .status {
      display: none;
      margin-top: 16px;
      padding: 12px 16px;
      border-radius: 8px;
      font-size: 0.82rem;
      line-height: 1.5;
      font-family: var(--mono);
    }
    .status.success { display: block; background: var(--success-bg); color: var(--accent); border: 1px solid var(--accent); }
    .status.error   { display: block; background: var(--error-bg);   color: var(--warn);   border: 1px solid var(--warn); }

    .footer {
      text-align: center;
      margin-top: 24px;
      font-size: 0.68rem;
      color: var(--muted);
      font-family: var(--mono);
      letter-spacing: 0.08em;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <div class="header-icon">&#x1F324;</div>
      <h1>Weather Clock</h1>
      <p>ESP32 &middot; 桌面天气时钟 &middot; 网络配置</p>
    </div>

    <div class="card">
      <form id="configForm" autocomplete="off">

        <!-- ── 01 WiFi 网络 ── -->
        <div class="section">
          <div class="section-label">01 &nbsp; WiFi 网络</div>

          <div class="scan-row">
            <div class="field">
              <label>WiFi 名称 (SSID)</label>
              <div class="input-wrap">
                <input type="text" id="ssid" name="ssid" placeholder="输入或点击扫描选择" required autocomplete="off">
              </div>
            </div>
            <button type="button" class="icon-btn" id="scanBtn" onclick="scanWifi()">
              <svg class="spin-icon" width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round">
                <path d="M1 4v6h6"/><path d="M3.51 15a9 9 0 1 0 .49-4.95"/>
              </svg>
              扫描
            </button>
          </div>

          <div class="drop-list" id="wifiList"></div>

          <div class="field">
            <label>WiFi 密码</label>
            <div class="input-wrap">
              <input type="password" id="password" name="password" placeholder="留空表示开放网络" autocomplete="new-password" style="padding-right:38px;">
              <button type="button" class="eye-btn" id="eyeBtn" onclick="togglePwd()" title="显示/隐藏密码">
                <svg id="eyeIcon" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round">
                  <path d="M1 12s4-7 11-7 11 7 11 7-4 7-11 7S1 12 1 12z"/>
                  <circle cx="12" cy="12" r="3"/>
                </svg>
              </button>
            </div>
          </div>
        </div>

        <!-- ── 02 天气配置 ── -->
        <div class="section">
          <div class="section-label">02 &nbsp; 天气配置</div>

          <div class="field">
            <label>和风天气 API Key</label>
            <div class="input-wrap">
              <input type="password" id="apikey" name="apikey"
                     value=""
                     placeholder="在 dev.qweather.com 免费申请；设备不会回显已保存的 Key" required autocomplete="new-password">
            </div>
          </div>

          <!-- 城市（本地列表，输入即搜） -->
          <div class="field">
            <label>城市</label>
            <div class="input-wrap">
              <input type="text" id="cityInput" placeholder="输入城市名即时搜索" autocomplete="off" oninput="onCityInput()">
            </div>
          </div>

          <div class="drop-list" id="cityList"></div>

          <!-- 已选城市徽章 -->
          <div class="city-badge" id="cityBadge">
            <span id="cityBadgeText"></span>
            <button type="button" class="clear-btn" onclick="clearCity()" title="重新选择">&#x2715;</button>
          </div>

          <!-- 隐藏字段：存 locationId，由城市搜索选择后填入 -->
          <input type="hidden" id="location" name="location" value="">
        </div>

        <button type="submit" class="submit-btn" id="submitBtn">保存并重启设备</button>
      </form>

      <div class="status" id="statusBox"></div>
    </div>

    <div class="footer">ESP32 Weather Clock &nbsp;&middot;&nbsp; 192.168.4.1</div>
  </div>

  <script>
    /* 内置中国主要城市（省会+地级市，和风天气 Location ID） */
    var CITIES=[{n:"上海",id:"101020100",p:"上海市"},{n:"临沧",id:"101291101",p:"云南省"},{n:"丽江",id:"101291401",p:"云南省"},{n:"保山",id:"101290501",p:"云南省"},{n:"大理州",id:"101290201",p:"云南省"},{n:"德宏",id:"101291501",p:"云南省"},{n:"怒江",id:"101291201",p:"云南省"},{n:"文山州",id:"101290601",p:"云南省"},{n:"昆明",id:"101290101",p:"云南省"},{n:"昭通",id:"101291001",p:"云南省"},{n:"普洱",id:"101290901",p:"云南省"},{n:"景洪",id:"101291601",p:"云南省"},{n:"曲靖",id:"101290401",p:"云南省"},{n:"楚雄州",id:"101290801",p:"云南省"},{n:"玉溪",id:"101290701",p:"云南省"},{n:"红河",id:"101290301",p:"云南省"},{n:"香格里拉",id:"101291301",p:"云南省"},{n:"临河",id:"101080801",p:"内蒙古自治区"},{n:"乌兰浩特",id:"101081101",p:"内蒙古自治区"},{n:"乌海",id:"101080301",p:"内蒙古自治区"},{n:"包头",id:"101080201",p:"内蒙古自治区"},{n:"呼和浩特",id:"101080101",p:"内蒙古自治区"},{n:"海拉尔",id:"101081001",p:"内蒙古自治区"},{n:"赤峰",id:"101080601",p:"内蒙古自治区"},{n:"通辽",id:"101080501",p:"内蒙古自治区"},{n:"鄂尔多斯",id:"101080701",p:"内蒙古自治区"},{n:"锡林浩特",id:"101080901",p:"内蒙古自治区"},{n:"阿左旗",id:"101081201",p:"内蒙古自治区"},{n:"集宁",id:"101080401",p:"内蒙古自治区"},{n:"北京",id:"101010100",p:"北京市"},{n:"吉林",id:"101060201",p:"吉林省"},{n:"四平",id:"101060401",p:"吉林省"},{n:"延吉",id:"101060301",p:"吉林省"},{n:"松原",id:"101060801",p:"吉林省"},{n:"白城",id:"101060601",p:"吉林省"},{n:"白山",id:"101060901",p:"吉林省"},{n:"辽源",id:"101060701",p:"吉林省"},{n:"通化",id:"101060501",p:"吉林省"},{n:"长春",id:"101060101",p:"吉林省"},{n:"乐山",id:"101271401",p:"四川省"},{n:"内江",id:"101271201",p:"四川省"},{n:"凉山",id:"101271601",p:"四川省"},{n:"南充",id:"101270501",p:"四川省"},{n:"宜宾",id:"101271101",p:"四川省"},{n:"巴中",id:"101270901",p:"四川省"},{n:"广元",id:"101272101",p:"四川省"},{n:"广安",id:"101270801",p:"四川省"},{n:"德阳",id:"101272001",p:"四川省"},{n:"成都",id:"101270101",p:"四川省"},{n:"攀枝花",id:"101270201",p:"四川省"},{n:"泸州",id:"101271001",p:"四川省"},{n:"甘孜",id:"101271801",p:"四川省"},{n:"眉山",id:"101271501",p:"四川省"},{n:"绵阳",id:"101270401",p:"四川省"},{n:"自贡",id:"101270301",p:"四川省"},{n:"资阳",id:"101271301",p:"四川省"},{n:"达州",id:"101270601",p:"四川省"},{n:"遂宁",id:"101270701",p:"四川省"},{n:"阿坝",id:"101271901",p:"四川省"},{n:"雅安",id:"101271701",p:"四川省"},{n:"天津",id:"101030100",p:"天津市"},{n:"中卫",id:"101170501",p:"宁夏回族自治区"},{n:"吴忠",id:"101170301",p:"宁夏回族自治区"},{n:"固原",id:"101170401",p:"宁夏回族自治区"},{n:"石嘴山",id:"101170201",p:"宁夏回族自治区"},{n:"银川",id:"101170101",p:"宁夏回族自治区"},{n:"亳州",id:"101220901",p:"安徽省"},{n:"六安",id:"101221501",p:"安徽省"},{n:"合肥",id:"101220101",p:"安徽省"},{n:"安庆",id:"101220601",p:"安徽省"},{n:"宣城",id:"101221401",p:"安徽省"},{n:"宿州",id:"101220701",p:"安徽省"},{n:"池州",id:"101221701",p:"安徽省"},{n:"淮北",id:"101221201",p:"安徽省"},{n:"淮南",id:"101220401",p:"安徽省"},{n:"滁州",id:"101221101",p:"安徽省"},{n:"芜湖",id:"101220301",p:"安徽省"},{n:"蚌埠",id:"101220201",p:"安徽省"},{n:"铜陵",id:"101221301",p:"安徽省"},{n:"阜阳",id:"101220801",p:"安徽省"},{n:"马鞍山",id:"101220501",p:"安徽省"},{n:"黄山",id:"101221001",p:"安徽省"},{n:"东营",id:"101121201",p:"山东省"},{n:"临沂",id:"101120901",p:"山东省"},{n:"威海",id:"101121301",p:"山东省"},{n:"德州",id:"101120401",p:"山东省"},{n:"日照",id:"101121501",p:"山东省"},{n:"枣庄",id:"101121401",p:"山东省"},{n:"泰安",id:"101120801",p:"山东省"},{n:"济南",id:"101120101",p:"山东省"},{n:"济宁",id:"101120701",p:"山东省"},{n:"淄博",id:"101120301",p:"山东省"},{n:"滨州",id:"101121101",p:"山东省"},{n:"潍坊",id:"101120601",p:"山东省"},{n:"烟台",id:"101120501",p:"山东省"},{n:"聊城",id:"101121701",p:"山东省"},{n:"菏泽",id:"101121001",p:"山东省"},{n:"青岛",id:"101120201",p:"山东省"},{n:"临汾",id:"101100701",p:"山西省"},{n:"吕梁",id:"101101100",p:"山西省"},{n:"大同",id:"101100201",p:"山西省"},{n:"太原",id:"101100101",p:"山西省"},{n:"忻州",id:"101101001",p:"山西省"},{n:"晋中",id:"101100401",p:"山西省"},{n:"晋城",id:"101100601",p:"山西省"},{n:"朔州",id:"101100901",p:"山西省"},{n:"运城",id:"101100801",p:"山西省"},{n:"长治",id:"101100501",p:"山西省"},{n:"阳泉",id:"101100301",p:"山西省"},{n:"东莞",id:"101281601",p:"广东省"},{n:"中山",id:"101281701",p:"广东省"},{n:"云浮",id:"101281401",p:"广东省"},{n:"佛山",id:"101280800",p:"广东省"},{n:"广州",id:"101280101",p:"广东省"},{n:"惠州",id:"101280301",p:"广东省"},{n:"揭阳",id:"101281901",p:"广东省"},{n:"梅州",id:"101280401",p:"广东省"},{n:"汕头",id:"101280501",p:"广东省"},{n:"汕尾",id:"101282101",p:"广东省"},{n:"江门",id:"101281101",p:"广东省"},{n:"河源",id:"101281201",p:"广东省"},{n:"深圳",id:"101280601",p:"广东省"},{n:"清远",id:"101281301",p:"广东省"},{n:"湛江",id:"101281001",p:"广东省"},{n:"潮州",id:"101281501",p:"广东省"},{n:"珠海",id:"101280701",p:"广东省"},{n:"肇庆",id:"101280901",p:"广东省"},{n:"茂名",id:"101282001",p:"广东省"},{n:"阳江",id:"101281801",p:"广东省"},{n:"韶关",id:"101280201",p:"广东省"},{n:"北海",id:"101301301",p:"广西壮族自治区"},{n:"南宁",id:"101300101",p:"广西壮族自治区"},{n:"崇左",id:"101300201",p:"广西壮族自治区"},{n:"来宾",id:"101300401",p:"广西壮族自治区"},{n:"柳州",id:"101300301",p:"广西壮族自治区"},{n:"桂林",id:"101300501",p:"广西壮族自治区"},{n:"梧州",id:"101300601",p:"广西壮族自治区"},{n:"河池",id:"101301201",p:"广西壮族自治区"},{n:"玉林",id:"101300901",p:"广西壮族自治区"},{n:"百色",id:"101301001",p:"广西壮族自治区"},{n:"贵港",id:"101300801",p:"广西壮族自治区"},{n:"贺州",id:"101300701",p:"广西壮族自治区"},{n:"钦州",id:"101301101",p:"广西壮族自治区"},{n:"防城港",id:"101301401",p:"广西壮族自治区"},{n:"乌鲁木齐",id:"101130101",p:"新疆维吾尔自治区"},{n:"五家渠",id:"101131801",p:"新疆维吾尔自治区"},{n:"伊宁",id:"101131001",p:"新疆维吾尔自治区"},{n:"克拉玛依",id:"101130201",p:"新疆维吾尔自治区"},{n:"北屯",id:"101132101",p:"新疆维吾尔自治区"},{n:"博乐",id:"101131601",p:"新疆维吾尔自治区"},{n:"双河",id:"101132201",p:"新疆维吾尔自治区"},{n:"可克达拉",id:"101132301",p:"新疆维吾尔自治区"},{n:"吐鲁番",id:"101130501",p:"新疆维吾尔自治区"},{n:"和田地区",id:"101131301",p:"新疆维吾尔自治区"},{n:"哈密",id:"101131201",p:"新疆维吾尔自治区"},{n:"喀什地区",id:"101130901",p:"新疆维吾尔自治区"},{n:"图木舒克",id:"101131701",p:"新疆维吾尔自治区"},{n:"塔城地区",id:"101131101",p:"新疆维吾尔自治区"},{n:"库尔勒",id:"101130601",p:"新疆维吾尔自治区"},{n:"新星市",id:"101132501",p:"新疆维吾尔自治区"},{n:"昆玉",id:"101131920",p:"新疆维吾尔自治区"},{n:"昌吉",id:"101130401",p:"新疆维吾尔自治区"},{n:"白杨市",id:"101132601",p:"新疆维吾尔自治区"},{n:"石河子",id:"101130301",p:"新疆维吾尔自治区"},{n:"胡杨河",id:"101132401",p:"新疆维吾尔自治区"},{n:"铁门关",id:"101131901",p:"新疆维吾尔自治区"},{n:"阿克苏",id:"101130801",p:"新疆维吾尔自治区"},{n:"阿勒泰地区",id:"101131401",p:"新疆维吾尔自治区"},{n:"阿图什",id:"101131501",p:"新疆维吾尔自治区"},{n:"阿拉尔",id:"101130701",p:"新疆维吾尔自治区"},{n:"南京",id:"101190101",p:"江苏省"},{n:"南通",id:"101190501",p:"江苏省"},{n:"宿迁",id:"101191301",p:"江苏省"},{n:"常州",id:"101191101",p:"江苏省"},{n:"徐州",id:"101190801",p:"江苏省"},{n:"扬州",id:"101190601",p:"江苏省"},{n:"无锡",id:"101190201",p:"江苏省"},{n:"泰州",id:"101191201",p:"江苏省"},{n:"淮安",id:"101190901",p:"江苏省"},{n:"盐城",id:"101190701",p:"江苏省"},{n:"苏州",id:"101190401",p:"江苏省"},{n:"连云港",id:"101191001",p:"江苏省"},{n:"镇江",id:"101190301",p:"江苏省"},{n:"上饶",id:"101240301",p:"江西省"},{n:"九江",id:"101240201",p:"江西省"},{n:"南昌",id:"101240101",p:"江西省"},{n:"吉安",id:"101240601",p:"江西省"},{n:"宜春",id:"101240501",p:"江西省"},{n:"抚州",id:"101240401",p:"江西省"},{n:"新余",id:"101241001",p:"江西省"},{n:"景德镇",id:"101240801",p:"江西省"},{n:"萍乡",id:"101240901",p:"江西省"},{n:"赣州",id:"101240701",p:"江西省"},{n:"鹰潭",id:"101241101",p:"江西省"},{n:"保定",id:"101090201",p:"河北省"},{n:"双桥",id:"101090401",p:"河北省"},{n:"唐山",id:"101090501",p:"河北省"},{n:"廊坊",id:"101090601",p:"河北省"},{n:"张家口",id:"101090301",p:"河北省"},{n:"沧州",id:"101090701",p:"河北省"},{n:"石家庄",id:"101090101",p:"河北省"},{n:"秦皇岛",id:"101091101",p:"河北省"},{n:"衡水",id:"101090801",p:"河北省"},{n:"邢台",id:"101090901",p:"河北省"},{n:"邯郸",id:"101091001",p:"河北省"},{n:"雄安新区",id:"101091201",p:"河北省"},{n:"三门峡",id:"101181701",p:"河南省"},{n:"信阳",id:"101180601",p:"河南省"},{n:"南阳",id:"101180701",p:"河南省"},{n:"周口",id:"101181401",p:"河南省"},{n:"商丘",id:"101181001",p:"河南省"},{n:"安阳",id:"101180201",p:"河南省"},{n:"平顶山",id:"101180501",p:"河南省"},{n:"开封",id:"101180801",p:"河南省"},{n:"新乡",id:"101180301",p:"河南省"},{n:"洛阳",id:"101180901",p:"河南省"},{n:"济源",id:"101181801",p:"河南省"},{n:"漯河",id:"101181501",p:"河南省"},{n:"濮阳",id:"101181301",p:"河南省"},{n:"焦作",id:"101181101",p:"河南省"},{n:"许昌",id:"101180401",p:"河南省"},{n:"郑州",id:"101180101",p:"河南省"},{n:"驻马店",id:"101181601",p:"河南省"},{n:"鹤壁",id:"101181201",p:"河南省"},{n:"丽水",id:"101210801",p:"浙江省"},{n:"台州",id:"101210601",p:"浙江省"},{n:"嘉兴",id:"101210301",p:"浙江省"},{n:"宁波",id:"101210401",p:"浙江省"},{n:"杭州",id:"101210101",p:"浙江省"},{n:"温州",id:"101210701",p:"浙江省"},{n:"湖州",id:"101210201",p:"浙江省"},{n:"舟山",id:"101211101",p:"浙江省"},{n:"衢州",id:"101211001",p:"浙江省"},{n:"越城",id:"101210501",p:"浙江省"},{n:"金华",id:"101210901",p:"浙江省"},{n:"万宁",id:"101310215",p:"海南省"},{n:"三亚",id:"101310201",p:"海南省"},{n:"三沙",id:"101310301",p:"海南省"},{n:"东方",id:"101310202",p:"海南省"},{n:"临高",id:"101310203",p:"海南省"},{n:"乐东",id:"101310221",p:"海南省"},{n:"五指山",id:"101310222",p:"海南省"},{n:"保亭",id:"101310214",p:"海南省"},{n:"儋州",id:"101310205",p:"海南省"},{n:"定安",id:"101310209",p:"海南省"},{n:"屯昌",id:"101310210",p:"海南省"},{n:"文昌",id:"101310212",p:"海南省"},{n:"昌江",id:"101310206",p:"海南省"},{n:"海口",id:"101310101",p:"海南省"},{n:"澄迈",id:"101310204",p:"海南省"},{n:"琼中",id:"101310208",p:"海南省"},{n:"琼海",id:"101310211",p:"海南省"},{n:"白沙",id:"101310207",p:"海南省"},{n:"陵水",id:"101310216",p:"海南省"},{n:"仙桃",id:"101201601",p:"湖北省"},{n:"十堰",id:"101201101",p:"湖北省"},{n:"咸宁",id:"101200701",p:"湖北省"},{n:"天门",id:"101201501",p:"湖北省"},{n:"孝感",id:"101200401",p:"湖北省"},{n:"宜昌",id:"101200901",p:"湖北省"},{n:"恩施州",id:"101201001",p:"湖北省"},{n:"武汉",id:"101200101",p:"湖北省"},{n:"潜江",id:"101201701",p:"湖北省"},{n:"神农架",id:"101201201",p:"湖北省"},{n:"荆州",id:"101200801",p:"湖北省"},{n:"荆门",id:"101201401",p:"湖北省"},{n:"襄阳",id:"101200201",p:"湖北省"},{n:"鄂州",id:"101200301",p:"湖北省"},{n:"随州",id:"101201301",p:"湖北省"},{n:"黄冈",id:"101200501",p:"湖北省"},{n:"黄石",id:"101200601",p:"湖北省"},{n:"吉首",id:"101251501",p:"湖南省"},{n:"娄底",id:"101250801",p:"湖南省"},{n:"岳阳",id:"101251001",p:"湖南省"},{n:"常德",id:"101250601",p:"湖南省"},{n:"张家界",id:"101251101",p:"湖南省"},{n:"怀化",id:"101251201",p:"湖南省"},{n:"株洲",id:"101250301",p:"湖南省"},{n:"永州",id:"101251401",p:"湖南省"},{n:"湘潭",id:"101250201",p:"湖南省"},{n:"益阳",id:"101250700",p:"湖南省"},{n:"衡阳",id:"101250401",p:"湖南省"},{n:"邵阳",id:"101250901",p:"湖南省"},{n:"郴州",id:"101250501",p:"湖南省"},{n:"长沙",id:"101250101",p:"湖南省"},{n:"临夏",id:"101161101",p:"甘肃省"},{n:"兰州",id:"101160101",p:"甘肃省"},{n:"合作",id:"101161201",p:"甘肃省"},{n:"嘉峪关",id:"101161401",p:"甘肃省"},{n:"天水",id:"101160901",p:"甘肃省"},{n:"定西",id:"101160201",p:"甘肃省"},{n:"平凉",id:"101160301",p:"甘肃省"},{n:"庆阳",id:"101160401",p:"甘肃省"},{n:"张掖",id:"101160701",p:"甘肃省"},{n:"武威",id:"101160501",p:"甘肃省"},{n:"武都",id:"101161001",p:"甘肃省"},{n:"白银",id:"101161301",p:"甘肃省"},{n:"酒泉",id:"101160801",p:"甘肃省"},{n:"金昌",id:"101160601",p:"甘肃省"},{n:"三明",id:"101230801",p:"福建省"},{n:"南平",id:"101230901",p:"福建省"},{n:"厦门",id:"101230201",p:"福建省"},{n:"宁德",id:"101230301",p:"福建省"},{n:"泉州",id:"101230501",p:"福建省"},{n:"漳州",id:"101230601",p:"福建省"},{n:"福州",id:"101230101",p:"福建省"},{n:"莆田",id:"101230401",p:"福建省"},{n:"龙岩",id:"101230701",p:"福建省"},{n:"山南",id:"101140301",p:"西藏自治区"},{n:"拉萨",id:"101140101",p:"西藏自治区"},{n:"日喀则",id:"101140201",p:"西藏自治区"},{n:"昌都",id:"101140501",p:"西藏自治区"},{n:"林芝",id:"101140401",p:"西藏自治区"},{n:"那曲",id:"101140601",p:"西藏自治区"},{n:"阿里地区",id:"101140701",p:"西藏自治区"},{n:"兴义",id:"101260901",p:"贵州省"},{n:"凯里",id:"101260501",p:"贵州省"},{n:"安顺",id:"101260301",p:"贵州省"},{n:"毕节",id:"101260701",p:"贵州省"},{n:"水城",id:"101260801",p:"贵州省"},{n:"贵阳",id:"101260101",p:"贵州省"},{n:"遵义",id:"101260201",p:"贵州省"},{n:"都匀",id:"101260401",p:"贵州省"},{n:"铜仁",id:"101260601",p:"贵州省"},{n:"丹东",id:"101070601",p:"辽宁省"},{n:"大连",id:"101070201",p:"辽宁省"},{n:"抚顺",id:"101070401",p:"辽宁省"},{n:"朝阳",id:"101071201",p:"辽宁省"},{n:"本溪",id:"101070501",p:"辽宁省"},{n:"沈阳",id:"101070101",p:"辽宁省"},{n:"盘锦",id:"101071301",p:"辽宁省"},{n:"营口",id:"101070801",p:"辽宁省"},{n:"葫芦岛",id:"101071401",p:"辽宁省"},{n:"辽阳",id:"101071001",p:"辽宁省"},{n:"铁岭",id:"101071101",p:"辽宁省"},{n:"锦州",id:"101070701",p:"辽宁省"},{n:"阜新",id:"101070901",p:"辽宁省"},{n:"鞍山",id:"101070301",p:"辽宁省"},{n:"重庆",id:"101040100",p:"重庆市"},{n:"咸阳",id:"101110200",p:"陕西省"},{n:"商洛",id:"101110601",p:"陕西省"},{n:"安康",id:"101110701",p:"陕西省"},{n:"宝鸡",id:"101110901",p:"陕西省"},{n:"延安",id:"101110300",p:"陕西省"},{n:"榆林",id:"101110401",p:"陕西省"},{n:"汉中",id:"101110801",p:"陕西省"},{n:"渭南",id:"101110501",p:"陕西省"},{n:"西安",id:"101110101",p:"陕西省"},{n:"铜川",id:"101111001",p:"陕西省"},{n:"共和",id:"101150401",p:"青海省"},{n:"同仁",id:"101150301",p:"青海省"},{n:"平安",id:"101150201",p:"青海省"},{n:"德令哈",id:"101150701",p:"青海省"},{n:"海晏",id:"101150801",p:"青海省"},{n:"玉树州",id:"101150601",p:"青海省"},{n:"玛沁",id:"101150501",p:"青海省"},{n:"西宁",id:"101150101",p:"青海省"},{n:"伊春",id:"101050801",p:"黑龙江省"},{n:"佳木斯",id:"101050401",p:"黑龙江省"},{n:"双鸭山",id:"101051301",p:"黑龙江省"},{n:"哈尔滨",id:"101050101",p:"黑龙江省"},{n:"大兴安岭",id:"101050701",p:"黑龙江省"},{n:"大庆",id:"101050901",p:"黑龙江省"},{n:"新兴",id:"101051001",p:"黑龙江省"},{n:"牡丹江",id:"101050301",p:"黑龙江省"},{n:"绥化",id:"101050501",p:"黑龙江省"},{n:"鸡西",id:"101051101",p:"黑龙江省"},{n:"鹤岗",id:"101051201",p:"黑龙江省"},{n:"黑河",id:"101050601",p:"黑龙江省"},{n:"齐齐哈尔",id:"101050201",p:"黑龙江省"}];

    /* ── 密码可见切换 ── */
    function togglePwd() {
      var inp = document.getElementById('password');
      var icon = document.getElementById('eyeIcon');
      if (inp.type === 'password') {
        inp.type = 'text';
        while (icon.firstChild) icon.removeChild(icon.firstChild);
        var p1 = document.createElementNS('http://www.w3.org/2000/svg','path');
        p1.setAttribute('d','M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24');
        var p2 = document.createElementNS('http://www.w3.org/2000/svg','line');
        p2.setAttribute('x1','1'); p2.setAttribute('y1','1'); p2.setAttribute('x2','23'); p2.setAttribute('y2','23');
        icon.appendChild(p1); icon.appendChild(p2);
      } else {
        inp.type = 'password';
        while (icon.firstChild) icon.removeChild(icon.firstChild);
        var p = document.createElementNS('http://www.w3.org/2000/svg','path');
        p.setAttribute('d','M1 12s4-7 11-7 11 7 11 7-4 7-11 7S1 12 1 12z');
        var c = document.createElementNS('http://www.w3.org/2000/svg','circle');
        c.setAttribute('cx','12'); c.setAttribute('cy','12'); c.setAttribute('r','3');
        icon.appendChild(p); icon.appendChild(c);
      }
    }

    /* ── 信号工具 ── */
    function rssiClass(rssi) {
      if (rssi >= -55) return 's4';
      if (rssi >= -65) return 's3';
      if (rssi >= -75) return 's2';
      return 's1';
    }
    function buildSigEl(cls) {
      var w = document.createElement('span');
      w.className = 'sig ' + cls;
      for (var i = 0; i < 4; i++) w.appendChild(document.createElement('span'));
      return w;
    }

    /* ── WiFi 扫描 ── */
    var scanInProgress = false;
    function scanWifi() {
      if (scanInProgress) return;
      scanInProgress = true;
      var btn  = document.getElementById('scanBtn');
      var list = document.getElementById('wifiList');
      btn.classList.add('spinning');
      btn.disabled = true;
      clearList(list);
      appendEmpty(list, '正在扫描...');
      list.classList.add('open');

      var xhr = new XMLHttpRequest();
      xhr.open('GET', '/scan');
      xhr.timeout = 15000;
      xhr.onload = function() {
        btn.classList.remove('spinning');
        btn.disabled = false;
        scanInProgress = false;
        clearList(list);
        try {
          var nets = JSON.parse(xhr.responseText);
          if (!nets.length) { appendEmpty(list, '未发现无线网络'); return; }
          nets.sort(function(a,b){ return b.rssi - a.rssi; });
          nets.forEach(function(n) {
            var item = document.createElement('div');
            item.className = 'drop-item';
            item.addEventListener('click', (function(name, el){ return function(){ selectWifi(el, name); }; })(n.ssid, item));

            var nameEl = document.createElement('span');
            nameEl.className = 'item-main';
            nameEl.textContent = n.ssid;

            var meta = document.createElement('span');
            meta.className = 'item-meta';
            if (n.enc) {
              var lock = document.createElement('span');
              lock.className = 'lock-icon';
              lock.textContent = '\uD83D\uDD12';
              meta.appendChild(lock);
            }
            meta.appendChild(buildSigEl(rssiClass(n.rssi)));
            var rssiEl = document.createElement('span');
            rssiEl.className = 'rssi-label';
            rssiEl.textContent = n.rssi + 'dBm';
            meta.appendChild(rssiEl);

            item.appendChild(nameEl);
            item.appendChild(meta);
            list.appendChild(item);
          });
        } catch(e) { appendEmpty(list, '解析失败，请重试'); }
      };
      xhr.onerror = xhr.ontimeout = function() {
        btn.classList.remove('spinning');
        btn.disabled = false;
        scanInProgress = false;
        clearList(list);
        appendEmpty(list, '扫描失败，请重试');
      };
      xhr.send();
    }

    function selectWifi(el, ssid) {
      document.getElementById('ssid').value = ssid;
      document.querySelectorAll('#wifiList .drop-item').forEach(function(i){ i.classList.remove('selected'); });
      el.classList.add('selected');
      document.getElementById('password').focus();
    }

    /* ── 内置城市列表（本地模糊搜索） ── */
    var cityInputTimer = null;
    function onCityInput() {
      if (cityInputTimer) clearTimeout(cityInputTimer);
      cityInputTimer = setTimeout(function() { doCitySearch(); }, 150);
    }
    function doCitySearch() {
      var q = document.getElementById('cityInput').value.trim().toLowerCase();
      var list = document.getElementById('cityList');
      clearList(list);
      if (!q) {
        list.classList.remove('open');
        return;
      }
      list.classList.add('open');
      var prefix = [];
      var contain = [];
      for (var i = 0; i < CITIES.length; i++) {
        var c = CITIES[i];
        var n = c.n.toLowerCase();
        var p = (c.p || '').toLowerCase();
        if (n.indexOf(q) !== -1 || p.indexOf(q) !== -1) {
          if (n.indexOf(q) === 0) prefix.push(c);
          else contain.push(c);
        }
      }
      var results = prefix.concat(contain);
      var max = 8;
      if (results.length === 0) {
        appendEmpty(list, '未找到匹配城市');
        return;
      }
      for (var j = 0; j < results.length && j < max; j++) {
        var c = results[j];
        var item = document.createElement('div');
        item.className = 'drop-item';
        item.addEventListener('click', (function(city, el){ return function(){ selectCity(el, city); }; })(c, item));
        var nameEl = document.createElement('span');
        nameEl.className = 'item-main';
        nameEl.textContent = c.n;
        var subEl = document.createElement('span');
        subEl.className = 'item-sub';
        subEl.textContent = c.p || '';
        item.appendChild(nameEl);
        item.appendChild(subEl);
        list.appendChild(item);
      }
    }

    function selectCity(el, city) {
      document.getElementById('location').value = city.id;
      document.querySelectorAll('#cityList .drop-item').forEach(function(i){ i.classList.remove('selected'); });
      el.classList.add('selected');
      var badge = document.getElementById('cityBadge');
      var badgeText = document.getElementById('cityBadgeText');
      badgeText.textContent = '\u2713  ' + city.n + '  (' + (city.p || '') + ')  ID: ' + city.id;
      badge.classList.add('show');
      document.getElementById('cityList').classList.remove('open');
    }

    function clearCity() {
      document.getElementById('location').value = '';
      document.getElementById('cityInput').value = '';
      document.getElementById('cityBadge').classList.remove('show');
      document.getElementById('cityList').classList.remove('open');
    }

    /* ── 工具 ── */
    function clearList(el) {
      while (el.firstChild) el.removeChild(el.firstChild);
    }
    function appendEmpty(el, msg) {
      var d = document.createElement('div');
      d.className = 'wifi-empty';
      d.textContent = msg;
      el.appendChild(d);
    }

    /* ── 表单提交 ── */
    document.getElementById('configForm').addEventListener('submit', function(e) {
      e.preventDefault();
      var statusBox = document.getElementById('statusBox');
      var submitBtn = document.getElementById('submitBtn');

      var locId = document.getElementById('location').value.trim();
      if (!locId) {
        statusBox.textContent = '请先搜索并选择城市';
        statusBox.className = 'status error';
        document.getElementById('cityInput').focus();
        return;
      }

      submitBtn.disabled = true;
      submitBtn.textContent = '正在保存...';
      statusBox.className = 'status';

      var data = JSON.stringify({
        ssid:     document.getElementById('ssid').value.trim(),
        password: document.getElementById('password').value,
        apikey:   document.getElementById('apikey').value.trim(),
        location: locId
      });

      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/save');
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.timeout = 8000;
      xhr.onload = function() {
        if (xhr.status === 200) {
          statusBox.textContent = '\u2713  配置已保存，设备正在重启，请重新连接目标 WiFi...';
          statusBox.className = 'status success';
          submitBtn.textContent = '重启中...';
        } else {
          statusBox.textContent = '\u2717  服务器返回错误，请重试';
          statusBox.className = 'status error';
          submitBtn.disabled = false;
          submitBtn.textContent = '保存并重启设备';
        }
      };
      xhr.onerror = function() {
        statusBox.textContent = '\u2717  请求失败，设备可能已重启，请重新连接';
        statusBox.className = 'status error';
        submitBtn.disabled = false;
        submitBtn.textContent = '保存并重启设备';
      };
      xhr.ontimeout = function() {
        statusBox.textContent = '\u2713  请求超时（设备已保存并重启），请重新连接目标 WiFi';
        statusBox.className = 'status success';
        submitBtn.textContent = '重启中...';
      };
      xhr.send(data);
    });
  </script>
</body>
</html>
)rawliteral";
