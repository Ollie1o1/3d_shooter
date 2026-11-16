// Sample the actual SDL Web Audio callback in a built release. No audio-device
// loopback or subjective listening claim: this checks the browser signal path.
// OVERDRIVE_PLAYWRIGHT=/path/to/playwright/index.mjs node tests/test_web_audio.mjs URL OUT.json
import fs from 'node:fs/promises';
const { chromium } = await import(process.env.OVERDRIVE_PLAYWRIGHT || 'playwright');
const url = new URL(process.argv[2] || 'http://127.0.0.1:8876/');
if (!url.searchParams.has('arena')) url.searchParams.set('arena', '1');
url.searchParams.set('god', '');
url.searchParams.set('res', '360');
const browser = await chromium.launch({ headless: true, args: ['--enable-unsafe-swiftshader'] });
try {
  const page = await browser.newPage({ viewport: { width: 1280, height: 720 } });
  page.setDefaultTimeout(120000);
  const errors = [];
  page.on('pageerror', error => errors.push(String(error)));
  await page.addInitScript(() => {
    // Match the visual smoke test's software-GPU pacing. This test measures
    // the real audio clock, not graphics throughput; queued draws must not
    // starve input or callback observation on a small headless runner.
    window.requestAnimationFrame = callback => setTimeout(() => callback(performance.now()), 250);
    window.cancelAnimationFrame = id => clearTimeout(id);
    localStorage.setItem('overdrive.settings.cfg', 'quality 0\nantiAlias 0\nambientDetail 0\nframeCap 2\n');
    // A real Web Audio clock with a silent sink: headless checks must not
    // depend on a sleeping laptop, HDMI monitor or CI sound device. Autoplay
    // policy and the actual SDL callback still run normally.
    const NativeAudioContext = window.AudioContext;
    window.AudioContext = class extends NativeAudioContext {
      constructor(options = {}) { super({ ...options, sinkId: { type: 'none' } }); }
    };
    window.audioStats = { samples: 0, sum: 0, peak: 0, bad: 0, rate: 0, active: false };
    window.observedAudioContexts = [];
    const create = AudioContext.prototype.createScriptProcessor;
    AudioContext.prototype.createScriptProcessor = function (...args) {
      const node = create.apply(this, args);
      const context = this;
      window.observedAudioContexts.push(context);
      // Observe after SDL installs its handler, whether it uses the property
      // or addEventListener. Leave the game's callback and its output intact.
      queueMicrotask(() => node.addEventListener('audioprocess', event => {
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
      }));
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
  // Actual input produces gun sounds alongside the live score (SUNSET by default, or a supplied checkpoint).
  for (let i = 0; i < 12; ++i) {
    await page.mouse.click(640, 360);
    await page.waitForTimeout(500);
  }
  // Input and weapon updates above are real. Once they have run, let Web
  // Audio finish the observation without submitting more software draws.
  await page.evaluate(() => Module.pauseMainLoop());
  await page.waitForFunction(() => window.audioStats.samples >= window.audioStats.rate * 2 * 6 && window.audioStats.rate > 0).catch(async error => {
    console.log(await page.evaluate(() => ({ stats: window.audioStats, contexts: window.observedAudioContexts.map(c => ({state: c.state, rate: c.sampleRate})) })));
    throw error;
  });
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
