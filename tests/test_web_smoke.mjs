// Run against a built release: OVERDRIVE_PLAYWRIGHT=/path/to/playwright/index.mjs
// node tests/test_web_smoke.mjs http://127.0.0.1:8765/ build/feel-pass/web
import fs from 'node:fs/promises';
import path from 'node:path';
const { chromium } = await import(process.env.OVERDRIVE_PLAYWRIGHT || 'playwright');
const url = process.argv[2] || 'http://127.0.0.1:8765/';
const output = process.argv[3] || 'build/feel-pass/web';
await fs.mkdir(output, { recursive: true });
const browser = await chromium.launch({
  ...(process.env.OVERDRIVE_BROWSER_CHANNEL ? { channel: process.env.OVERDRIVE_BROWSER_CHANNEL } : {}),
  headless: true, args: ['--enable-unsafe-swiftshader'],
});
try {
  const page = await browser.newPage({ viewport: { width: 1280, height: 720 } });
  page.setDefaultTimeout(120000); // software-rendered frames can delay input on a two-core runner
  // Software WebGL in CI has no GPU. Keep the real shaders/lighting enabled,
  // but use 720p without MSAA so input and screenshots don't queue behind
  // supersampled frames. This fresh context never touches a player's settings.
  await page.addInitScript(() => localStorage.setItem('overdrive.settings.cfg',
    'quality 0\nantiAlias 0\nambientDetail 1\nframeCap 2\n'));
  const errors = [];
  page.on('pageerror', error => errors.push(String(error)));
  page.on('response', response => {
    if (response.status() >= 400 && !response.url().includes('favicon'))
      errors.push(`${response.status()} ${response.url()}`);
  });
  page.on('console', message => {
    if (message.type() === 'error' && !/favicon|404|api\/overdrive-board/.test(message.text())) errors.push(message.text());
  });
  // Local static servers have no leaderboard API. Stub only that optional API.
  await page.route('**/api/overdrive-board**', route => route.fulfill({ json: { entries: [] } }));
  async function enterGame() {
    // Pointer lock may arrive as initialization finishes, in which case the
    // shipped shell dismisses the overlay itself. Clicking a locator races
    // that removal and waits forever for a button that is already gone.
    await page.waitForFunction(() => {
      const overlay = document.querySelector('#overlay');
      return !overlay || overlay.dataset.state === 'ready';
    });
    await page.mouse.click(640, 360); // real user gesture, on the overlay or the running canvas
    await page.locator('#overlay').waitFor({ state: 'detached' });
  }
  async function capture(name) {
    // Stop producing frames while Chromium drains software WebGL commands
    // for the screenshot. Input and simulation run normally between captures;
    // a continuous stream of draws can starve capture on a two-core runner.
    await page.evaluate(() => Module.pauseMainLoop());
    try {
      await page.screenshot({ path: path.join(output, name) });
    } finally {
      await page.evaluate(() => Module.resumeMainLoop());
    }
  }
  await page.goto(url, { waitUntil: 'domcontentloaded' });
  await enterGame();
  await page.keyboard.press('F3');
  await page.waitForTimeout(600);
  await capture('f3-guide.png');
  // Select each lesson through actual keyboard input. Opening and closing the
  // guide repeatedly also exercises pointer-lock/pause restoration in WebAssembly.
  for (let lesson = 0; lesson < 20; ++lesson) {
    await page.keyboard.press('Enter');
    await page.waitForTimeout(450);
    if ([7, 8, 11, 13, 19].includes(lesson))
      await capture(`lesson-${lesson}.png`);
    await page.keyboard.press('F4');
    await page.waitForTimeout(70);
    if (lesson < 19) await page.keyboard.press('ArrowDown');
  }
  await page.keyboard.press('Home');
  await page.keyboard.press('PageDown');
  await capture('guide-page-2.png');
  await page.keyboard.press('PageDown');
  await capture('guide-page-3.png');
  console.log('Training guide and twenty lesson input paths exercised.');
  // The moving-platform chapter exercises the new material shader in WebGL,
  // alongside live waves, rather than only the training room's static walls.
  const reliquary = new URL(url);
  reliquary.searchParams.set('act2', ''); reliquary.searchParams.set('arena', '4');
  reliquary.searchParams.set('god', '');
  await page.goto(reliquary.href, { waitUntil: 'domcontentloaded' });
  await enterGame();
  await page.keyboard.press('Tab'); // later-arena starts open the upgrade armory
  await page.waitForTimeout(6500);
  await capture('reliquary.png');
  console.log('Reliquary rendered.');
  for (const [name, params] of [
    ['challenge', { challenge: 'OD1-12345-2' }],
    ['phase-practice', { boss: '2', phase: '2' }],
    ['boss-rush', { bossrush: '' }],
    ['motor-assessment', { story3: '', arena: '3', god: '' }],
    ['blast-chillers', { story5: '', arena: '6', god: '' }],
    ['dispatch-2', { story5: '', arena: '8', god: '' }],
    ['vault', { story5: '', arena: '10', god: '' }],
  ]) {
    const target = new URL(url); target.search = new URLSearchParams(params).toString();
    await page.goto(target.href, { waitUntil: 'domcontentloaded' });
    await enterGame();
    if (['phase-practice', 'boss-rush'].includes(name)) await page.keyboard.press('Tab');
    if (['blast-chillers', 'dispatch-2', 'vault'].includes(name)) {
      // Story checkpoint starts enter gameplay directly. Enter also resumes
      // an initial browser pointer-lock pause without opening an armory.
      await page.locator('#canvas').focus();
      await page.waitForTimeout(750);
      await page.keyboard.press('Enter');
      // SDL handles Enter on its next frame. If that arrives after the
      // browser's gesture expires, use the shipped Click to aim affordance
      // for a fresh pointer gesture once the game wants capture again.
      await page.waitForFunction(() => document.pointerLockElement || document.querySelector('#aim').hasAttribute('data-on'));
      await page.mouse.click(640, 360);
      // Some headless browsers deny capture. The game must remain active
      // with Click to aim, rather than pausing after an unheld lock is denied.
      await page.waitForTimeout(500);
      await page.waitForFunction(() => document.pointerLockElement || document.querySelector('#aim').hasAttribute('data-on'));
      await page.keyboard.down('w');
      await page.waitForTimeout(1500);
      await page.keyboard.up('w');
    }
    await page.waitForTimeout(3500);
    await capture(`${name}.png`);
    console.log(`${name} rendered.`);
    if (name === 'phase-practice') {
      await page.keyboard.press('Backspace');
      await page.waitForTimeout(1000);
    }
    if (name === 'challenge') {
      await page.keyboard.press('Escape');
      await capture('challenge-code.png');
    }
  }
  if (errors.length) throw new Error(errors.join('\n'));
  console.log('WebAssembly boots; all twenty lessons, guide paging, Reliquary, seeded challenges, phase practice, boss rush, Motor Assessment and three Cold Store combat spaces render without browser errors.');
} finally {
  await browser.close();
}
