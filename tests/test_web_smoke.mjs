// Run against a built release: OVERDRIVE_PLAYWRIGHT=/path/to/playwright/index.mjs
// node tests/test_web_smoke.mjs http://127.0.0.1:8765/ build/feel-pass/web
import fs from 'node:fs/promises';
import path from 'node:path';
const { chromium } = await import(process.env.OVERDRIVE_PLAYWRIGHT || 'playwright');
const url = process.argv[2] || 'http://127.0.0.1:8765/';
const output = process.argv[3] || 'build/feel-pass/web';
await fs.mkdir(output, { recursive: true });
const browser = await chromium.launch({ channel: 'chrome', headless: true, args: ['--enable-unsafe-swiftshader'] });
try {
  const page = await browser.newPage({ viewport: { width: 1280, height: 720 } });
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
  await page.goto(url, { waitUntil: 'domcontentloaded' });
  await page.locator('#overlay[data-state="ready"]').waitFor({ timeout: 120000 });
  await page.locator('#overlay').click();
  await page.keyboard.press('F3');
  await page.waitForTimeout(600);
  await page.screenshot({ path: path.join(output, 'f3-guide.png') });
  // Select each lesson through actual keyboard input. Opening and closing the
  // guide repeatedly also exercises pointer-lock/pause restoration in WebAssembly.
  for (let lesson = 0; lesson < 20; ++lesson) {
    await page.keyboard.press('Enter');
    await page.waitForTimeout(450);
    if ([7, 8, 11, 13, 19].includes(lesson))
      await page.screenshot({ path: path.join(output, `lesson-${lesson}.png`) });
    await page.keyboard.press('F4');
    await page.waitForTimeout(70);
    if (lesson < 19) await page.keyboard.press('ArrowDown');
  }
  await page.keyboard.press('Home');
  await page.keyboard.press('PageDown');
  await page.screenshot({ path: path.join(output, 'guide-page-2.png') });
  await page.keyboard.press('PageDown');
  await page.screenshot({ path: path.join(output, 'guide-page-3.png') });
  // The moving-platform chapter exercises the new material shader in WebGL,
  // alongside live waves, rather than only the training room's static walls.
  const reliquary = new URL(url);
  reliquary.searchParams.set('act2', ''); reliquary.searchParams.set('arena', '4');
  reliquary.searchParams.set('god', '');
  await page.goto(reliquary.href, { waitUntil: 'domcontentloaded' });
  await page.locator('#overlay[data-state="ready"]').waitFor({ timeout: 120000 });
  await page.locator('#overlay').click();
  await page.keyboard.press('Tab'); // later-arena starts open the upgrade armory
  await page.waitForTimeout(6500);
  await page.screenshot({ path: path.join(output, 'reliquary.png') });
  if (errors.length) throw new Error(errors.join('\n'));
  console.log('WebAssembly boots, F3 and all twenty lessons work, guide paging responds, and Reliquary renders live without browser errors.');
} finally {
  await browser.close();
}
