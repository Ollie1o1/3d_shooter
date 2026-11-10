// Sample the actual SDL Web Audio callback in a built release. No audio-device
// loopback or subjective listening claim: this checks the browser signal path.
// OVERDRIVE_PLAYWRIGHT=/path/to/playwright/index.mjs node tests/test_web_audio.mjs URL OUT.json
import fs from 'node:fs/promises';
const { chromium } = await import(process.env.OVERDRIVE_PLAYWRIGHT || 'playwright');
const url = new URL(process.argv[2] || 'http://127.0.0.1:8876/');
url.searchParams.set('arena', '1');
url.searchParams.set('god', '');
url.searchParams.set('res', '360');
const browser = await chromium.launch({ headless: true, args: ['--enable-unsafe-swiftshader'] });
try {
  const page = await browser.newPage({ viewport: { width: 1280, height: 720 } });
  page.setDefaultTimeout(120000);
  const errors = [];
  page.on('pageerror', error => errors.push(String(error)));
  await page.addInitScript(() => {
    window.requestAnimationFrame = callback => setTimeout(() => callback(performance.now()), 100);
    window.cancelAnimationFrame = id => clearTimeout(id);
    localStorage.setItem('overdrive.settings.cfg', 'quality 0\nantiAlias 0\nambientDetail 0\nframeCap 2\n');
    window.audioStats = { samples: 0, sum: 0, peak: 0, bad: 0, rate: 0, active: false };
    const descriptor = Object.getOwnPropertyDescriptor(ScriptProcessorNode.prototype, 'onaudioprocess');
    const create = AudioContext.prototype.createScriptProcessor;
    AudioContext.prototype.createScriptProcessor = function (...args) {
      const node = create.apply(this, args);
      const context = this;
      let callback = null;
      Object.defineProperty(node, 'onaudioprocess', {
        configurable: true,
        get() { return callback; },
        set(fn) {
          callback = fn;
          descriptor.set.call(node, fn ? function (event) {
            fn.call(node, event);
            const stats = window.audioStats;
            if (!stats.active) return;
            stats.rate = context.sampleRate;
            for (let channel = 0; channel < event.outputBuffer.numberOfChannels; ++channel) {
              for (const sample of event.outputBuffer.getChannelData(channel)) {
                stats.samples++;
                if (!Number.isFinite(sample)) stats.bad++;
                stats.peak = Math.max(stats.peak, Math.abs(sample));
                stats.sum += sample * sample;
              }
            }
          } : null);
        },
      });
      return node;
    };
  });
  await page.route('**/api/overdrive-board**', route => route.fulfill({ json: { entries: [] } }));
  await page.goto(url.href, { waitUntil: 'domcontentloaded' });
  await page.waitForFunction(() => {
    const overlay = document.querySelector('#overlay');
    return !overlay || overlay.dataset.state === 'ready';
  });
  await page.mouse.click(640, 360);
  await page.locator('#overlay').waitFor({ state: 'detached' });
  await page.evaluate(() => { window.audioStats.active = true; });
  // Actual input produces gun sounds alongside the live SUNSET score.
  for (let i = 0; i < 12; ++i) {
    await page.mouse.click(640, 360);
    await page.waitForTimeout(500);
  }
  await page.waitForFunction(() => window.audioStats.samples >= window.audioStats.rate * 2 * 6 && window.audioStats.rate > 0);
  const stats = await page.evaluate(() => ({ ...window.audioStats }));
  stats.rms = Math.sqrt(stats.sum / stats.samples);
  if (errors.length || stats.bad || stats.rms < .0001 || stats.peak > .701)
    throw new Error(JSON.stringify({ stats, errors }));
  const result = { ...stats, errors, url: url.href };
  if (process.argv[3]) await fs.writeFile(process.argv[3], JSON.stringify(result, null, 2) + '\n');
  console.log(`Web Audio: ${stats.rate} Hz, ${stats.samples} samples, RMS ${stats.rms.toFixed(4)}, peak ${stats.peak.toFixed(4)}, no non-finite samples.`);
} finally {
  await browser.close();
}
