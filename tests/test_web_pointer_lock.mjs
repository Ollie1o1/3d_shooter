// Run the actual shipped shell wrapper with browsers that reject raw input,
// deny both lock attempts, detach the canvas, or cancel while a lock is pending.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
const html=fs.readFileSync(new URL('../web/index.html',import.meta.url),'utf8');
const code=html.slice(html.indexOf('  let wantsMouse = false;'),html.indexOf('  // Dismiss on the first interaction'));
assert.ok(code.includes('requestPointerLock'));
function browser(request) {
  class Document {exitPointerLock(){this.pointerLockElement=null;}addEventListener(){}getElementById(){return {dataset:{}};}}
  const document=new Document();
  class Canvas {isConnected=true;ownerDocument=document;requestPointerLock(opts){return request(opts);}}
  const context=vm.createContext({document,HTMLCanvasElement:Canvas,Document,canvas:new Canvas(),
    window:{addEventListener(){}},overlay:{dataset:{state:'ready'}},setInterval(){},setTimeout(){},Promise});
  vm.runInContext(code,context);return context;
}
let calls=0;
const denied=browser(()=>{++calls;return Promise.reject(new Error('WrongDocumentError'));});
await vm.runInContext('canvas.requestPointerLock()',denied);
assert.equal(calls,2,'both denied promises settle without escaping to the caller');
calls=0;
const unsupported=browser(opts=>{++calls;if(opts)throw new Error('unsupported raw input');throw new Error('inactive document');});
await vm.runInContext('canvas.requestPointerLock()',unsupported);assert.equal(calls,2);
calls=0;
const detached=browser(()=>{++calls;return Promise.resolve();});
vm.runInContext('canvas.isConnected=false',detached);await vm.runInContext('canvas.requestPointerLock()',detached);assert.equal(calls,0);
calls=0;let reject;
const pending=browser(()=>{++calls;return new Promise((_,fail)=>{reject=fail;});});
const waiting=vm.runInContext('canvas.requestPointerLock()',pending);
vm.runInContext('window.overdriveMouseWanted(false)',pending);reject(new Error('cancelled'));await waiting;
assert.equal(calls,1,'releasing the cursor cancels fallback reacquisition');
const success=browser(()=>Promise.resolve('locked'));
assert.equal(await vm.runInContext('canvas.requestPointerLock()',success),'locked');
console.log('Pointer-lock wrapper handles success, rejected fallbacks, synchronous errors, detached documents and cancellation.');
