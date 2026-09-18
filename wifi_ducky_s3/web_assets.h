#pragma once
#include <Arduino.h>

static const char PAGE_INDEX[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>Dongle</title>
<style>
:root{
  --bg:#FAFAF7; --card:#FFFFFF; --line:#EAEAE4;
  --ink:#1A1D1B; --muted:#6E7671; --faint:#9AA29C;
  --accent:#5E8C61; --accent-hover:#4E7751; --accent-soft:#EDF3ED;
  --warn:#B4761F; --danger:#B3453C;
  --shadow:0 1px 2px rgba(26,29,27,.04), 0 2px 8px rgba(26,29,27,.05);
  --mono:ui-monospace,SFMono-Regular,Menlo,Consolas,"Liberation Mono",monospace;
  --sans:system-ui,-apple-system,"Segoe UI",Roboto,Inter,sans-serif;
  --r:14px;
}
*{box-sizing:border-box}
html,body{margin:0}
body{
  background:var(--bg); color:var(--ink);
  font:15px/1.55 var(--sans);
  padding-bottom:4rem;
  -webkit-text-size-adjust:100%;
}
h1{font-size:28px;font-weight:600;letter-spacing:-.02em;margin:0}
h2{font-size:16px;font-weight:600;letter-spacing:-.01em;margin:0}
p.sub{margin:.3rem 0 0;color:var(--muted);font-size:14px;max-width:56ch}
button{font:inherit;color:inherit;background:none;border:none;cursor:pointer}
:focus-visible{outline:2px solid var(--accent);outline-offset:2px;border-radius:6px}
[hidden]{display:none!important}

/* ---- top bar ---- */
.topbar{
  position:sticky;top:0;z-index:10;
  display:flex;align-items:center;justify-content:space-between;
  padding:.85rem 1.25rem;
  background:rgba(250,250,247,.88);backdrop-filter:blur(10px);
  border-bottom:1px solid var(--line);
}
.brand{display:flex;align-items:center;gap:.6rem;font-weight:600;letter-spacing:-.01em}
.dot{width:8px;height:8px;border-radius:50%;background:var(--faint);flex:none;transition:background .2s}
.dot.on{background:var(--accent)}
.dot.busy{background:var(--warn)}
.tools{display:flex;align-items:center;gap:.5rem}
.chip{
  padding:.3rem .7rem;border-radius:999px;
  background:var(--accent-soft);color:var(--accent);
  font-size:12.5px;font-weight:600;letter-spacing:.01em;
}
.chip:hover{background:#E3EDE3}
.iconbtn{
  width:36px;height:36px;border-radius:50%;
  display:grid;place-items:center;color:var(--muted);
}
.iconbtn:hover{background:#F0F0EB;color:var(--ink)}
.iconbtn svg{width:20px;height:20px;fill:none;stroke:currentColor;stroke-width:1.7}

main{max-width:880px;margin:0 auto;padding:1.75rem 1.25rem 0}
.pagehead{margin-bottom:1.25rem}

/* ---- cards ---- */
.card{
  background:var(--card);border:1px solid var(--line);border-radius:var(--r);
  box-shadow:var(--shadow);margin-bottom:1rem;overflow:hidden;
}
.card-hd{
  display:flex;align-items:center;justify-content:space-between;gap:1rem;
  padding:1rem 1.15rem;
}
.card-bd{padding:0 1.15rem 1.15rem}
.card-hd + .card-bd{padding-top:0}

/* ---- payload library ---- */
.layout{display:grid;grid-template-columns:240px 1fr;gap:1rem;align-items:start}
.files{list-style:none;margin:0;padding:0 .5rem .5rem}
.files button{
  width:100%;text-align:left;padding:.55rem .65rem;border-radius:9px;
  font:13px/1.4 var(--mono);color:var(--muted);
  overflow:hidden;text-overflow:ellipsis;white-space:nowrap;
}
.files button{display:flex;align-items:center;gap:.5rem}
.files button .nm{overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.osicon{width:15px;height:15px;flex:none;opacity:.65}
.files button[aria-current="true"] .osicon{opacity:1}
.osicon.spacer{visibility:hidden}
.files button:hover{background:#F5F5F0;color:var(--ink)}
.files button[aria-current="true"]{background:var(--accent-soft);color:var(--accent);font-weight:600}
.blank{padding:.5rem .65rem 1rem;color:var(--faint);font-size:13px}

/* ---- editor ---- */
.namefield{
  flex:1;min-width:0;
  background:var(--bg);border:1px solid var(--line);border-radius:9px;
  padding:.45rem .65rem;font:13px var(--mono);color:var(--ink);
}
.namefield:focus{outline:none;border-color:var(--accent);background:#fff}
textarea{
  display:block;width:100%;height:320px;resize:vertical;
  background:var(--bg);color:var(--ink);
  border:1px solid var(--line);border-radius:10px;
  padding:.85rem;font:13px/1.7 var(--mono);
  white-space:pre;overflow-wrap:normal;overflow-x:auto;
}
textarea:focus{outline:none;border-color:var(--accent);background:#fff}

.runbar{display:flex;align-items:center;gap:.65rem;padding-top:.9rem;flex-wrap:wrap}
.btn{
  padding:.6rem 1.25rem;border-radius:999px;font-weight:600;font-size:14.5px;
  background:var(--accent);color:#fff;transition:background .15s;
}
.btn:hover:enabled{background:var(--accent-hover)}
.btn:disabled{background:#DEDED8;color:#9AA29C;cursor:not-allowed}
.btn.ghost{background:transparent;color:var(--muted);border:1px solid var(--line)}
.btn.ghost:hover:enabled{background:#F5F5F0;color:var(--ink)}
.btn.ghost.stop:enabled{color:var(--danger);border-color:#E7CFCD}
.btn.danger{background:transparent;color:var(--danger);border:1px solid #E7CFCD}
.btn.danger:hover{background:#FDF3F2}
.link{color:var(--accent);font-size:13.5px;font-weight:500;padding:.25rem .4rem;border-radius:7px}
.link:hover{background:var(--accent-soft)}
.link.quiet{color:var(--muted)}
.link.quiet:hover{background:#F0F0EB;color:var(--ink)}
.delay{display:flex;align-items:center;gap:.4rem;font-size:13.5px;color:var(--muted)}
.delay input{
  width:64px;background:var(--bg);color:var(--ink);
  border:1px solid var(--line);border-radius:999px;
  padding:.42rem .6rem;font:13.5px var(--sans);text-align:center;
}
.delay input:focus{outline:none;border-color:var(--accent);background:#fff}
.status{margin-left:auto;font-size:13px;color:var(--muted);font-variant-numeric:tabular-nums}
.status.err{color:var(--danger)}
.status.ok{color:var(--accent)}

pre{
  margin:0;padding:.9rem 1rem;max-height:200px;overflow:auto;
  background:var(--bg);border:1px solid var(--line);border-radius:10px;
  font:12.5px/1.65 var(--mono);color:var(--muted);white-space:pre-wrap;
}

/* ---- settings ---- */
.back{display:inline-flex;align-items:center;gap:.4rem;color:var(--muted);font-size:14px;margin-bottom:1rem;padding:.3rem .5rem;border-radius:8px}
.back:hover{background:#F0F0EB;color:var(--ink)}
.preview{display:flex;justify-content:center;margin:1.1rem 0 .3rem}
.lchoices{
  margin-top:.9rem;border:1px solid var(--line);border-radius:12px;overflow:hidden;
  max-height:310px;overflow-y:auto;
}
.lrow{
  width:100%;display:flex;align-items:center;justify-content:space-between;gap:1rem;
  padding:.7rem .9rem;border-top:1px solid var(--line);text-align:left;
}
.lrow:first-child{border-top:none}
.lrow:hover{background:#F5F5F0}
.lrow[aria-checked="true"]{background:var(--accent-soft)}
.lrow[aria-checked="true"] .lname{color:var(--accent);font-weight:600}
.lname{font-size:14px}
.larr{
  font:11px/1 var(--mono);letter-spacing:.04em;
  color:var(--muted);background:var(--bg);
  border:1px solid var(--line);border-radius:999px;padding:.28rem .55rem;flex:none;
}
.lrow[aria-checked="true"] .larr{color:var(--accent);border-color:#C3D5C4;background:#fff}
.choices{display:grid;grid-template-columns:1fr 1fr;gap:.75rem;margin-top:1rem}
.choice{
  padding:1rem .8rem .85rem;border-radius:12px;
  border:1.5px solid var(--line);background:var(--card);
  display:flex;flex-direction:column;align-items:center;gap:.7rem;
  transition:border-color .15s,background .15s;
}
.choice:hover{border-color:#CFD6CF}
.choice[aria-checked="true"]{border-color:var(--accent);background:var(--accent-soft)}
.caps{display:flex;gap:4px}
.cap{
  width:29px;height:33px;display:grid;place-items:center;
  font:12.5px/1 var(--mono);color:var(--muted);
  background:#fff;border:1px solid var(--line);border-bottom-width:2.5px;border-radius:6px;
}
.choice[aria-checked="true"] .cap{color:var(--accent);border-color:#C3D5C4}
.caplab{font-size:13.5px;color:var(--muted)}
.choice[aria-checked="true"] .caplab{color:var(--ink);font-weight:500}

/* ---- toggles, orientation, swatches ---- */
.row{
  display:flex;align-items:center;justify-content:space-between;gap:1rem;
  padding:.85rem 0;border-top:1px solid var(--line);
}
.row:first-of-type{border-top:none}
.row .lab{font-size:14.5px}
.row .lab small{display:block;color:var(--muted);font-size:12.5px;margin-top:.1rem}
.switch{
  position:relative;width:46px;height:27px;border-radius:999px;
  background:#DEDED8;flex:none;transition:background .18s;
}
.switch[aria-checked="true"]{background:var(--accent)}
.switch::after{
  content:"";position:absolute;top:3px;left:3px;width:21px;height:21px;
  border-radius:50%;background:#fff;transition:transform .18s;
  box-shadow:0 1px 3px rgba(0,0,0,.18);
}
.switch[aria-checked="true"]::after{transform:translateX(19px)}

.orient{display:grid;grid-template-columns:repeat(4,1fr);gap:.6rem;margin-top:.9rem}
.orient button{
  padding:.8rem .4rem .6rem;border-radius:11px;
  border:1.5px solid var(--line);background:var(--card);
  display:flex;flex-direction:column;align-items:center;gap:.5rem;
  font-size:12px;color:var(--muted);
}
.orient button:hover{border-color:#CFD6CF}
.orient button[aria-checked="true"]{border-color:var(--accent);background:var(--accent-soft);color:var(--ink)}
.glyph{
  border:1.5px solid var(--faint);border-radius:3px;position:relative;background:#fff;
}
.orient button[aria-checked="true"] .glyph{border-color:var(--accent)}
.glyph.land{width:34px;height:19px}
.glyph.port{width:19px;height:34px}
/* the bar marks the top edge, so a flipped tile is readable at a glance */
.glyph::before{content:"";position:absolute;left:2px;right:2px;height:3px;background:var(--faint);top:2px}
.glyph.flip::before{top:auto;bottom:2px}
.orient button[aria-checked="true"] .glyph::before{background:var(--accent)}

.sdbar{display:flex;align-items:center;gap:.5rem;margin-top:1rem;flex-wrap:wrap}
.sdpath{font:12.5px var(--mono);color:var(--muted);flex:1;min-width:0;
  overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.sdlist{border:1px solid var(--line);border-radius:12px;overflow:hidden;margin-top:.6rem;
  max-height:260px;overflow-y:auto}
.sdrow{display:flex;align-items:center;gap:.6rem;padding:.55rem .8rem;border-top:1px solid var(--line)}
.sdrow:first-child{border-top:none}
.sdrow .fn{flex:1;min-width:0;font:13px/1.4 var(--mono);
  overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.sdrow .fs{font-size:12px;color:var(--faint);flex:none}
.sdrow.dir .fn{color:var(--accent);cursor:pointer}
.sdrow .act{font-size:12.5px;color:var(--muted);padding:.2rem .4rem;border-radius:6px;flex:none}
.sdrow .act:hover{background:#F0F0EB;color:var(--ink)}
.sdrow .act.del:hover{background:#FDF3F2;color:var(--danger)}
.sdnote{padding:.9rem .8rem;color:var(--faint);font-size:13px}
.swatches{display:flex;gap:.7rem;flex-wrap:wrap;margin-top:.9rem;align-items:center}
.swatches .sw{
  width:30px;height:30px;flex:none;padding:0;border-radius:50%;
  border:1px solid rgba(26,29,27,.16);
}
/* Ring drawn outside the circle, so it reads on any swatch colour
   including white. */
.swatches .sw[aria-checked="true"],.pickwrap.on{
  box-shadow:0 0 0 2px var(--card),0 0 0 4px var(--accent);
}
.pickwrap{
  width:34px;height:34px;flex:none;border-radius:50%;padding:2px;
  display:grid;place-items:center;cursor:pointer;
  background:conic-gradient(#E5484D,#F5B942,#5DD08A,#5AC8FA,#8C6BB1,#E5484D);
}
.swatches .picker{
  width:30px;height:30px;padding:0;border:none;border-radius:50%;
  background:none;cursor:pointer;-webkit-appearance:none;appearance:none;
}
.picker::-webkit-color-swatch-wrapper{padding:0}
.picker::-webkit-color-swatch{border:none;border-radius:50%}
.picker::-moz-color-swatch{border:none;border-radius:50%}

.field{margin-top:1rem}
.field label{display:block;font-size:13.5px;font-weight:500;margin-bottom:.35rem}
/* Text fields only. A bare ".field input" also caught the colour picker
   and the "show password" checkbox and stretched them to full width. */
.field input[type="text"],.field input[type="password"]{
  width:100%;background:var(--bg);color:var(--ink);
  border:1px solid var(--line);border-radius:10px;
  padding:.65rem .8rem;font:14.5px var(--sans);
}
.field input[type="text"]:focus,.field input[type="password"]:focus{
  outline:none;border-color:var(--accent);background:#fff;
}
.field .hint{font-size:12.5px;color:var(--faint);margin-top:.35rem}
.reveal{display:flex;align-items:center;gap:.4rem;margin-top:.5rem;font-size:13px;color:var(--muted)}
.notice{
  margin-top:1rem;padding:.75rem .9rem;border-radius:10px;
  background:#FCF6EA;color:#7A5312;font-size:13.5px;
}

footer{max-width:880px;margin:2rem auto 0;padding:0 1.25rem;color:var(--faint);font-size:12.5px}
footer #ver{font:11.5px var(--mono)}

/* ---- reconnect overlay ---- */
.overlay{
  position:fixed;inset:0;z-index:50;display:grid;place-items:center;
  background:rgba(250,250,247,.96);padding:2rem;text-align:center;
}
.overlay h2{font-size:21px;margin-bottom:.6rem}
.overlay p{color:var(--muted);max-width:34ch;margin:0 auto}
.overlay .net{
  display:inline-block;margin-top:1.1rem;padding:.5rem 1rem;border-radius:999px;
  background:var(--accent-soft);color:var(--accent);font:14px var(--mono);font-weight:600;
}

@media (max-width:760px){
  .layout{grid-template-columns:1fr}
  textarea{height:240px}
  h1{font-size:24px}
  main{padding-top:1.25rem}
}
@media (prefers-reduced-motion:reduce){*{transition:none!important}}
</style>
</head>
<body>

<div class="topbar">
  <div class="brand"><span class="dot" id="dot"></span>Dongle</div>
  <div class="tools">
    <button class="chip" id="layoutchip" title="Keyboard layout">AZERTY</button>
    <button class="iconbtn" id="gear" aria-label="Settings">
      <svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="3.2"/><path d="M19.4 13.5a7.7 7.7 0 0 0 0-3l1.7-1.3-1.8-3.1-2 .8a7.7 7.7 0 0 0-2.6-1.5L14.4 3h-3.6l-.3 2.4a7.7 7.7 0 0 0-2.6 1.5l-2-.8L4 9.2l1.7 1.3a7.7 7.7 0 0 0 0 3L4 14.8l1.8 3.1 2-.8a7.7 7.7 0 0 0 2.6 1.5l.3 2.4h3.6l.3-2.4a7.7 7.7 0 0 0 2.6-1.5l2 .8 1.8-3.1z"/></svg>
    </button>
  </div>
</div>

<!-- ============ main ============ -->
<main id="view-main">
  <div class="pagehead">
    <h1>Payloads</h1>
    <p class="sub">Pick a script and run it on the machine the dongle is plugged into.</p>
  </div>

  <div class="layout">
    <section class="card">
      <div class="card-hd"><h2>Library</h2><button class="link" id="new">New</button></div>
      <ul class="files" id="list"></ul>
    </section>

    <section class="card">
      <div class="card-hd">
        <input type="text" class="namefield" id="name" placeholder="payload-name.txt" spellcheck="false" autocapitalize="off">
        <button class="link" id="save">Save</button>
        <button class="link quiet" id="del">Delete</button>
      </div>
      <div class="card-bd">
        <textarea id="script" spellcheck="false" autocapitalize="off" autocomplete="off"
          placeholder="REM One command per line&#10;GUI r&#10;DELAY 500&#10;STRING notepad&#10;ENTER"></textarea>
        <div class="runbar">
          <button class="btn" id="run">Run</button>
          <button class="btn ghost stop" id="stop" disabled>Stop</button>
          <label class="delay">Start after
            <input type="number" id="delay" min="0" max="3600" step="1" value="0"
                   aria-label="Seconds before the payload starts">
            s
          </label>
          <span class="status" id="status">Ready</span>
        </div>
      </div>
    </section>
  </div>

  <section class="card">
    <div class="card-hd"><h2>Run log</h2><button class="link quiet" id="clearlog">Clear</button></div>
    <div class="card-bd"><pre id="log">Nothing yet. The log fills up on the first run.</pre></div>
  </section>
</main>

<!-- ============ settings ============ -->
<main id="view-settings" hidden>
  <button class="back" id="back">&larr; Payloads</button>
  <div class="pagehead">
    <h1>Settings</h1>
  </div>

  <section class="card">
    <div class="card-bd" style="padding-top:1.15rem">
      <h2>Keyboard layout</h2>
      <p class="sub">The dongle sends key positions, not letters. Match this to the keyboard of the machine it is plugged into, or your payload types gibberish.</p>
      <div class="preview"><span class="caps" id="caps"></span></div>
      <div class="lchoices" role="radiogroup" aria-label="Keyboard layout" id="lchoices"></div>
    </div>
  </section>

  <section class="card">
    <div class="card-bd" style="padding-top:1.15rem">
      <h2>Display and light</h2>
      <p class="sub">Orientation, backlight and the status LED on the dongle itself.</p>

      <div class="orient" role="radiogroup" aria-label="Screen orientation">
        <button role="radio" aria-checked="false" data-rot="1"><span class="glyph land"></span>Landscape</button>
        <button role="radio" aria-checked="false" data-rot="3"><span class="glyph land flip"></span>Flipped</button>
        <button role="radio" aria-checked="false" data-rot="0"><span class="glyph port"></span>Portrait</button>
        <button role="radio" aria-checked="false" data-rot="2"><span class="glyph port flip"></span>Flipped</button>
      </div>

      <div class="row" style="margin-top:1.1rem">
        <span class="lab">Screen<small>Tapping the button on the dongle lights it up for a few seconds either way.</small></span>
        <button class="switch" id="screensw" role="switch" aria-checked="true" aria-label="Screen"></button>
      </div>

      <div class="row">
        <span class="lab">Status LED<small>Still turns amber while a payload runs, and red on an error.</small></span>
        <button class="switch" id="ledsw" role="switch" aria-checked="true" aria-label="Status LED"></button>
      </div>

      <div id="ledcolorrow">
        <div class="field" style="margin-top:.4rem">
          <label>Resting colour</label>
          <div class="swatches" id="swatches">
            <button class="sw" data-c="#005A8C" style="background:#005A8C" aria-checked="false" aria-label="Blue"></button>
            <button class="sw" data-c="#5E8C61" style="background:#5E8C61" aria-checked="false" aria-label="Green"></button>
            <button class="sw" data-c="#8C6BB1" style="background:#8C6BB1" aria-checked="false" aria-label="Purple"></button>
            <button class="sw" data-c="#C77D2E" style="background:#C77D2E" aria-checked="false" aria-label="Amber"></button>
            <button class="sw" data-c="#B3453C" style="background:#B3453C" aria-checked="false" aria-label="Red"></button>
            <button class="sw" data-c="#FFFFFF" style="background:#FFFFFF" aria-checked="false" aria-label="White"></button>
            <span class="pickwrap" id="pickwrap" title="Custom colour"><input type="color" class="picker" id="picker" aria-label="Custom colour"></span>
          </div>
        </div>
      </div>

      <div class="runbar">
        <button class="btn" id="savedisplay">Apply</button>
        <span class="status" id="dispstatus"></span>
      </div>
    </div>
  </section>

  <section class="card">
    <div class="card-bd" style="padding-top:1.15rem">
      <h2>Device name</h2>
      <p class="sub">Shown on the dongle's screen, and used as the drive's
      model name so a payload can find it. On Linux it turns up in
      <code>/dev/disk/by-id/</code> and under <code>lsblk -o MODEL</code>.</p>
      <div class="field">
        <label for="devname">Name</label>
        <input type="text" id="devname" maxlength="16" spellcheck="false" autocapitalize="off">
        <div class="hint">Up to 16 characters. The screen updates at once;
        the drive's model name changes the next time the dongle is plugged in.</div>
      </div>
      <div class="runbar">
        <button class="btn" id="savename">Save name</button>
        <span class="status" id="namestatus"></span>
      </div>
    </div>
  </section>

  <section class="card">
    <div class="card-bd" style="padding-top:1.15rem">
      <h2>USB drive</h2>
      <p class="sub">Presents the microSD card to the machine as a removable
      drive, alongside the keyboard. The reader itself is always advertised;
      this decides whether it reports a card.</p>

      <div class="row" style="margin-top:.6rem">
        <span class="lab">Expose the card<small id="drivehint">Checking the slot...</small></span>
        <button class="switch" id="drivesw" role="switch" aria-checked="false" aria-label="Expose the card"></button>
      </div>
      <span class="status" id="drivestatus" style="margin-left:0"></span>

      <div class="sdbar">
        <button class="link quiet" id="sdup">&uarr; Up</button>
        <span class="sdpath" id="sdpath">/</span>
        <button class="link" id="sdrefresh">Refresh</button>
      </div>
      <div class="sdlist" id="sdlist"></div>
    </div>
  </section>

  <section class="card">
    <div class="card-bd" style="padding-top:1.15rem">
      <h2>Wi-Fi network</h2>
      <p class="sub">This is the network the dongle creates. Changing it restarts the device, so you will need to join the new network to get back here.</p>

      <div class="field">
        <label for="ssid">Network name</label>
        <input type="text" id="ssid" maxlength="32" spellcheck="false" autocapitalize="off" autocomplete="off">
      </div>

      <div class="field">
        <label for="pass">Password</label>
        <input type="password" id="pass" maxlength="63" spellcheck="false" autocapitalize="off" autocomplete="new-password">
        <div class="hint" id="passhint">8 to 63 characters.</div>
        <label class="reveal"><input type="checkbox" id="reveal"> Show password</label>
      </div>

      <div class="runbar">
        <button class="btn" id="savewifi">Save and restart</button>
        <span class="status" id="wifistatus"></span>
      </div>
    </div>
  </section>

  <section class="card">
    <div class="card-bd" style="padding-top:1.15rem">
      <h2>Reset</h2>
      <p class="sub">Restores the built-in network name and password, and the French layout. Your payloads are kept.</p>
      <div class="notice">Locked out? Hold the button on the dongle for five seconds. It restores the same defaults without needing this page.</div>
      <div class="runbar"><button class="btn danger" id="reset">Restore defaults</button></div>
    </div>
  </section>
</main>

<footer>Machines you own, or have written authorisation to test.
<span id="ver"></span></footer>

<!-- ============ reconnect overlay ============ -->
<div class="overlay" id="overlay" hidden>
  <div>
    <h2>The dongle is restarting</h2>
    <p>Join this network again to get back to the app.</p>
    <div class="net" id="newnet"></div>
  </div>
</div>

<script>
const $ = s => document.querySelector(s);
const list = $('#list'), nameIn = $('#name'), script = $('#script');
const runBtn = $('#run'), stopBtn = $('#stop'), status = $('#status'), logEl = $('#log');
let current = null, layout = 'us', dirty = false, poll = true;

script.addEventListener('input', () => dirty = true);

async function api(path, opts) {
  const r = await fetch(path, opts);
  if (!r.ok) {
    let msg = r.status;
    try { msg = (await r.json()).error || msg; } catch (e) {}
    throw new Error(msg);
  }
  return r;
}
function form(obj) {
  const b = new URLSearchParams();
  for (const k in obj) b.append(k, obj[k]);
  return { method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:b };
}

/* ---- navigation ---- */
function show(view) {
  const settings = view === 'settings';
  $('#view-main').hidden = settings;
  $('#view-settings').hidden = !settings;
  if (settings) loadSettings();
  window.scrollTo(0, 0);
}
$('#gear').onclick = () => show('settings');
$('#layoutchip').onclick = () => show('settings');
$('#back').onclick = () => show('main');

/* ---- layout ---- */
let layouts = [], arrangement = 'QWERTY';

function paintLayout() {
  $('#layoutchip').textContent = arrangement;

  const caps = ['AZERTY', 'QWERTZ', 'QWERTY'].includes(arrangement) ? arrangement : 'QWERTY';
  const capsEl = $('#caps');
  if (capsEl) {
    capsEl.innerHTML = '';
    for (const ch of caps) {
      const s = document.createElement('span');
      s.className = 'cap';
      s.textContent = ch;
      capsEl.appendChild(s);
    }
  }

  const box = $('#lchoices');
  if (!box || !layouts.length) return;
  box.innerHTML = '';
  for (const l of layouts) {
    const b = document.createElement('button');
    b.className = 'lrow';
    b.setAttribute('role', 'radio');
    b.setAttribute('aria-checked', l.code === layout);
    b.innerHTML = '<span class="lname"></span><span class="larr"></span>';
    b.querySelector('.lname').textContent = l.name;
    b.querySelector('.larr').textContent = l.arrangement;
    b.onclick = async () => {
      layout = l.code;
      arrangement = l.arrangement;
      paintLayout();
      try { await api('/api/layout', form({layout})); }
      catch (e) { $('#wifistatus').textContent = e.message; }
    };
    box.appendChild(b);
  }
}

/* ---- payloads ---- */
const OS_ICON = {
  windows: '<svg class="osicon" viewBox="0 0 16 16" fill="currentColor" aria-hidden="true"><rect x="1" y="1.6" width="6" height="6" rx=".8"/><rect x="9" y="1.6" width="6" height="6" rx=".8"/><rect x="1" y="9.4" width="6" height="6" rx=".8"/><rect x="9" y="9.4" width="6" height="6" rx=".8"/></svg>',
  macos: '<svg class="osicon" viewBox="0 0 16 16" fill="currentColor" aria-hidden="true"><path d="M11.2 8.5c0-1.6 1.3-2.4 1.4-2.4-.8-1.1-2-1.3-2.4-1.3-1-.1-2 .6-2.5.6s-1.3-.6-2.2-.6c-1.1 0-2.2.7-2.7 1.7-1.2 2-.3 5 .8 6.6.6.8 1.2 1.7 2.1 1.7.9 0 1.2-.5 2.2-.5s1.3.5 2.2.5 1.5-.8 2-1.6c.7-.9.9-1.8.9-1.9 0 0-1.8-.7-1.8-2.8zM9.7 3.5c.5-.6.8-1.4.7-2.2-.7 0-1.6.5-2.1 1.1-.4.5-.8 1.3-.7 2.1.8.1 1.6-.4 2.1-1z"/></svg>',
  linux: '<svg class="osicon" viewBox="0 0 16 16" fill="currentColor" aria-hidden="true"><path d="M8 .9C6.2.9 5 2.3 5 4v1.5c0 .7-.3 1.1-.8 1.8C3.3 8.5 2.4 9.9 2.4 11.3c0 1.5 1 2.6 2.1 3.2.4.2.6.5.7.9h5.6c.1-.4.3-.7.7-.9 1.1-.6 2.1-1.7 2.1-3.2 0-1.4-.9-2.8-1.8-4-.5-.7-.8-1.1-.8-1.8V4c0-1.7-1.2-3.1-3-3.1zM6.8 3.4c.4 0 .7.4.7 1s-.3 1-.7 1-.7-.4-.7-1 .3-1 .7-1zm2.4 0c.4 0 .7.4.7 1s-.3 1-.7 1-.7-.4-.7-1 .3-1 .7-1zM8 5.8c.6 0 1.3.4 1.3.8 0 .3-.8.8-1.3.8s-1.3-.5-1.3-.8c0-.4.7-.8 1.3-.8z"/></svg>',
};

function paintList(items) {
  list.innerHTML = '';
  if (!items.length) {
    list.innerHTML = '<li class="blank">No payloads yet. Create one to get started.</li>';
    return;
  }
  for (const it of items) {
    const li = document.createElement('li');
    const b = document.createElement('button');
    b.innerHTML = (OS_ICON[it.os] || '<svg class="osicon spacer" viewBox="0 0 16 16"></svg>')
                + '<span class="nm"></span>';
    b.querySelector('.nm').textContent = it.name;
    if (it.os) b.title = it.name + ' \u2014 ' + it.os;
    b.setAttribute('aria-current', it.name === current);
    b.onclick = () => openPayload(it.name);
    li.appendChild(b);
    list.appendChild(li);
  }
}

async function openPayload(n) {
  if (dirty && !confirm('The current payload has unsaved changes. Discard them?')) return;
  const r = await api('/api/payload?name=' + encodeURIComponent(n));
  script.value = await r.text();
  nameIn.value = n;
  current = n;
  dirty = false;
  refresh();
}

$('#new').onclick = () => {
  current = null;
  nameIn.value = '';
  script.value = 'REM New payload\n';
  dirty = false;
  nameIn.focus();
  refresh();
};

$('#save').onclick = async () => {
  const n = nameIn.value.trim();
  if (!n) { status.textContent = 'Name the payload first'; nameIn.focus(); return; }
  try {
    await api('/api/payload', form({name:n, content:script.value}));
    current = n; dirty = false;
    status.textContent = 'Saved';
    refresh();
  } catch (e) { status.textContent = e.message; }
};

$('#del').onclick = async () => {
  if (!current) { status.textContent = 'No payload selected'; return; }
  if (!confirm('Delete ' + current + '?')) return;
  await api('/api/payload/delete', form({name:current}));
  current = null; nameIn.value = ''; script.value = ''; dirty = false;
  refresh();
};

runBtn.onclick = async () => {
  try {
    await api('/api/run', form({script:script.value, delay:$('#delay').value || 0}));
    refresh();
  } catch (e) { status.textContent = e.message; }
};
stopBtn.onclick = () => api('/api/stop', form({}));
$('#clearlog').onclick = async () => { await api('/api/log/clear', form({})); refresh(); };

/* ---- settings ---- */
let rotation = 1, screenOn = true, ledOn = true, ledColor = '#005A8C';

function paintDisplay() {
  document.querySelectorAll('.orient button').forEach(b =>
    b.setAttribute('aria-checked', +b.dataset.rot === rotation));
  $('#screensw').setAttribute('aria-checked', screenOn);
  $('#ledsw').setAttribute('aria-checked', ledOn);
  $('#ledcolorrow').hidden = !ledOn;
  const up = ledColor.toUpperCase();
  let preset = false;
  document.querySelectorAll('.sw').forEach(b => {
    const hit = b.dataset.c.toUpperCase() === up;
    if (hit) preset = true;
    b.setAttribute('aria-checked', hit);
  });
  $('#pickwrap').classList.toggle('on', !preset);
  $('#picker').value = ledColor;
}

document.querySelectorAll('.orient button').forEach(b => b.onclick = () => {
  rotation = +b.dataset.rot; paintDisplay();
});
$('#screensw').onclick = () => { screenOn = !screenOn; paintDisplay(); };

let usbDrive = false, sdPath = '/';

$('#savename').onclick = async () => {
  const st = $('#namestatus');
  st.className = 'status';
  try {
    await api('/api/settings/name', form({name: $('#devname').value.trim()}));
    st.className = 'status ok';
    st.textContent = 'Saved';
  } catch (e) { st.className = 'status err'; st.textContent = e.message; }
};

function fmtSize(n) {
  if (n < 1024) return n + ' B';
  if (n < 1024 * 1024) return (n / 1024).toFixed(1) + ' KB';
  return (n / 1048576).toFixed(1) + ' MB';
}

async function sdBrowse(path) {
  const box = $('#sdlist');
  try {
    const r = await api('/api/sd/list?path=' + encodeURIComponent(path));
    const d = await r.json();
    sdPath = d.path;
    $('#sdpath').textContent = sdPath;
    box.innerHTML = '';
    if (!d.entries.length) {
      box.innerHTML = '<div class="sdnote">Empty folder.</div>';
      return;
    }
    d.entries.sort((a, b) => (b.dir - a.dir) || a.name.localeCompare(b.name));
    for (const e of d.entries) {
      const row = document.createElement('div');
      row.className = 'sdrow' + (e.dir ? ' dir' : '');
      const full = (sdPath === '/' ? '' : sdPath) + '/' + e.name;

      const nm = document.createElement('span');
      nm.className = 'fn';
      nm.textContent = e.dir ? e.name + '/' : e.name;
      if (e.dir) nm.onclick = () => sdBrowse(full);
      row.appendChild(nm);

      const sz = document.createElement('span');
      sz.className = 'fs';
      sz.textContent = e.dir ? '' : fmtSize(e.size);
      row.appendChild(sz);

      if (!e.dir) {
        const dl = document.createElement('button');
        dl.className = 'act';
        dl.textContent = 'Download';
        dl.onclick = () => { window.location = '/api/sd/download?path=' + encodeURIComponent(full); };
        row.appendChild(dl);
      }

      const rm = document.createElement('button');
      rm.className = 'act del';
      rm.textContent = 'Delete';
      rm.onclick = async () => {
        if (!confirm('Delete ' + e.name + '?')) return;
        try { await api('/api/sd/delete', form({path: full})); sdBrowse(sdPath); }
        catch (err) { box.innerHTML = '<div class="sdnote">' + err.message + '</div>'; }
      };
      row.appendChild(rm);

      box.appendChild(row);
    }
  } catch (e) {
    box.innerHTML = '<div class="sdnote">' + e.message + '</div>';
  }
}

$('#sdrefresh').onclick = () => sdBrowse(sdPath);
$('#sdup').onclick = () => {
  if (sdPath === '/') return;
  const up = sdPath.substring(0, sdPath.lastIndexOf('/')) || '/';
  sdBrowse(up);
};
$('#drivesw').onclick = async () => {
  usbDrive = !usbDrive;
  $('#drivesw').setAttribute('aria-checked', usbDrive);
  const st = $('#drivestatus');
  st.className = 'status';
  try {
    await api('/api/settings/drive', form({exposed: usbDrive ? 1 : 0}));
    st.textContent = usbDrive ? 'Attached to the host' : 'Detached';
    sdBrowse('/');
  } catch (e) {
    st.className = 'status err';
    st.textContent = e.message;
    usbDrive = false;
    $('#drivesw').setAttribute('aria-checked', false);
  }
};
$('#ledsw').onclick = () => { ledOn = !ledOn; paintDisplay(); };
document.querySelectorAll('.sw').forEach(b => b.onclick = () => {
  ledColor = b.dataset.c; paintDisplay();
});
$('#picker').oninput = e => { ledColor = e.target.value; paintDisplay(); };

$('#savedisplay').onclick = async () => {
  const st = $('#dispstatus');
  st.className = 'status';
  try {
    await api('/api/settings/display', form({
      rotation, screen: screenOn ? 1 : 0, led: ledOn ? 1 : 0, ledColor
    }));
    st.className = 'status ok';
    st.textContent = 'Applied';
  } catch (e) { st.className = 'status err'; st.textContent = e.message; }
};

async function loadSettings() {
  try {
    const s = await (await api('/api/settings')).json();
    $('#ssid').value = s.ssid;
    $('#pass').value = s.password;
    $('#passhint').textContent = s.passwordMin + ' to ' + s.passwordMax + ' characters.';
    layout = s.layout;
    layouts = s.layouts || layouts;
    rotation = s.rotation;
    screenOn = !!s.screen;
    ledOn = !!s.led;
    ledColor = s.ledColor;
    if (document.activeElement !== $('#delay')) $('#delay').value = s.startDelay;
    if (document.activeElement !== $('#devname')) $('#devname').value = s.deviceName;
    usbDrive = !!s.usbDrive;
    $('#drivesw').setAttribute('aria-checked', usbDrive);
    $('#drivehint').textContent = s.usbCard
      ? ('Card detected, ' + s.usbSizeMB + ' MB.')
      : 'No card in the slot.';
    sdBrowse(sdPath);
    paintLayout();
    paintDisplay();
  } catch (e) { $('#wifistatus').textContent = 'Could not load settings'; }
}

$('#reveal').onchange = e => { $('#pass').type = e.target.checked ? 'text' : 'password'; };

function restarting(net) {
  poll = false;
  $('#newnet').textContent = net;
  $('#overlay').hidden = false;
}

$('#savewifi').onclick = async () => {
  const ssid = $('#ssid').value, password = $('#pass').value;
  const ws = $('#wifistatus');
  ws.className = 'status';
  try {
    await api('/api/settings/wifi', form({ssid, password}));
    restarting(ssid);
  } catch (e) { ws.className = 'status err'; ws.textContent = e.message; }
};

$('#reset').onclick = async () => {
  if (!confirm('Restore the built-in network name, password and layout?')) return;
  try {
    await api('/api/settings/reset', form({}));
    restarting('the built-in network shown on the screen');
  } catch (e) { $('#wifistatus').textContent = e.message; }
};

/* ---- polling ---- */
function paintState(s) {
  const armed = s.state === 'armed';
  const busy = armed || s.state === 'running';
  runBtn.disabled = busy;
  stopBtn.disabled = !busy;
  $('#delay').disabled = busy;
  $('#dot').className = 'dot ' + (busy ? 'busy' : 'on');

  status.className = 'status' + (s.state === 'error' ? ' err' : s.state === 'done' ? ' ok' : '');
  if (armed) status.textContent = 'Starting in ' + s.countdown + ' s';
  else if (s.state === 'running') status.textContent = 'Line ' + s.line + ' of ' + s.total;
  else status.textContent = s.message || 'Ready';

  if (s.layout !== layout || s.arrangement !== arrangement) {
    layout = s.layout;
    arrangement = s.arrangement;
    paintLayout();
  }
  logEl.textContent = s.log || 'Nothing yet. The log fills up on the first run.';
  if (s.version) $('#ver').textContent = 'v' + s.version;
}

async function refresh() {
  if (!poll) return;
  let s;
  try {
    s = await (await api('/api/state')).json();
  } catch (e) {
    $('#dot').className = 'dot';   // offline: the device stopped answering
    return;
  }
  try { paintState(s); } catch (e) { console.error('paintState', e); }
  try { paintList(s.payloads); } catch (e) { console.error('paintList', e); }
}

setInterval(refresh, 1000);
paintLayout();
refresh();
// Seeds the remembered delay without opening the settings view.
api('/api/settings').then(r => r.json())
  .then(s => { $('#delay').value = s.startDelay; })
  .catch(() => {});
</script>
</body>
</html>)HTML";
