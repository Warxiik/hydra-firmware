import { useState } from 'react';
import { Flame, Crosshair, ArrowDown, ArrowUp, Thermometer, RotateCcw } from 'lucide-react';
import type { PrinterState, PrinterControls } from '../hooks/usePrinter';

const PRESETS = [
  { name: 'PLA',  nozzle: 210, bed: 60,  icon: '🟢' },
  { name: 'PETG', nozzle: 235, bed: 80,  icon: '🔵' },
  { name: 'ABS',  nozzle: 245, bed: 100, icon: '🟠' },
  { name: 'TPU',  nozzle: 225, bed: 50,  icon: '🟣' },
];

function TempInput({ label, value, color, onSet }: {
  label: string; value: number; color: string; onSet: (temp: number) => void;
}) {
  const [inputVal, setInputVal] = useState(value.toString());

  return (
    <div className="rounded-xl p-4" style={{ background: 'var(--color-surface)', border: `1px solid ${color}20` }}>
      <div className="flex items-center gap-2 mb-3">
        <Thermometer size={14} style={{ color }} />
        <span className="text-xs font-semibold tracking-wide" style={{ color, fontFamily: 'var(--font-display)' }}>
          {label}
        </span>
        <span className="text-xs ml-auto" style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-mono)' }}>
          {value.toFixed(0)}°C
        </span>
      </div>
      <div className="flex gap-2">
        <input
          type="number"
          value={inputVal}
          onChange={e => setInputVal(e.target.value)}
          className="flex-1 h-11 rounded-lg px-3 text-sm outline-none"
          style={{
            background: 'var(--color-card)',
            border: '1px solid var(--color-border)',
            color: 'var(--color-text)',
            fontFamily: 'var(--font-mono)',
            fontWeight: 600,
          }}
          min={0} max={300}
        />
        <button
          onClick={() => onSet(Number(inputVal))}
          className="h-11 px-4 rounded-lg text-xs font-semibold transition-all active:scale-95 cursor-pointer"
          style={{ background: `${color}25`, color, border: `1px solid ${color}40`, fontFamily: 'var(--font-display)' }}>
          Set
        </button>
        <button
          onClick={() => { onSet(0); setInputVal('0'); }}
          className="h-11 px-3 rounded-lg transition-all active:scale-95 cursor-pointer"
          style={{ background: 'var(--color-card)', border: '1px solid var(--color-border)' }}>
          <RotateCcw size={14} style={{ color: 'var(--color-text-dim)' }} />
        </button>
      </div>
    </div>
  );
}

export function PreparePage({ state, controls }: { state: PrinterState; controls: PrinterControls }) {
  return (
    <div className="flex-1 overflow-y-auto p-5 animate-fade-in">
      <div className="max-w-[1100px] mx-auto grid grid-cols-2 gap-5">
        {/* Preheat presets */}
        <div className="rounded-2xl p-5"
          style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
          <h3 className="text-xs font-semibold tracking-widest uppercase mb-4"
            style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-display)' }}>
            <Flame size={12} className="inline mr-2" style={{ color: 'var(--color-accent)' }} />
            Preheat Presets
          </h3>
          <div className="grid grid-cols-2 gap-3">
            {PRESETS.map(p => (
              <button key={p.name}
                onClick={() => {
                  controls.setNozzleTemp(0, p.nozzle);
                  controls.setNozzleTemp(1, p.nozzle);
                  controls.setBedTemp(p.bed);
                }}
                className="flex items-center gap-3 p-4 rounded-xl transition-all active:scale-95 cursor-pointer text-left"
                style={{ background: 'var(--color-surface)', border: '1px solid var(--color-border)' }}
                onMouseEnter={e => { e.currentTarget.style.borderColor = 'var(--color-accent)'; }}
                onMouseLeave={e => { e.currentTarget.style.borderColor = 'var(--color-border)'; }}
              >
                <span className="text-xl">{p.icon}</span>
                <div>
                  <span className="text-sm font-semibold block" style={{ fontFamily: 'var(--font-display)' }}>{p.name}</span>
                  <span className="text-[10px]" style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-mono)' }}>
                    {p.nozzle}° / {p.bed}°
                  </span>
                </div>
              </button>
            ))}
          </div>
        </div>

        {/* Home & movement */}
        <div className="rounded-2xl p-5"
          style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
          <h3 className="text-xs font-semibold tracking-widest uppercase mb-4"
            style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-display)' }}>
            <Crosshair size={12} className="inline mr-2" style={{ color: 'var(--color-accent)' }} />
            Home Axes
          </h3>
          <div className="grid grid-cols-2 gap-3 mb-5">
            <button onClick={controls.homeAll}
              className="col-span-2 h-14 rounded-xl text-sm font-semibold transition-all active:scale-95 cursor-pointer flex items-center justify-center gap-2"
              style={{ background: 'var(--color-accent)15', border: '1px solid var(--color-accent)30', color: 'var(--color-accent)', fontFamily: 'var(--font-display)' }}>
              <Crosshair size={16} /> Home All (G28)
            </button>
            {['X', 'Y', 'Z'].map(axis => (
              <button key={axis}
                onClick={() => controls.homeAxis(axis as 'X' | 'Y' | 'Z')}
                className="h-12 rounded-xl text-sm font-semibold transition-all active:scale-95 cursor-pointer"
                style={{ background: 'var(--color-surface)', border: '1px solid var(--color-border)', fontFamily: 'var(--font-mono)' }}>
                Home {axis}
              </button>
            ))}
          </div>

          <h3 className="text-xs font-semibold tracking-widest uppercase mb-3"
            style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-display)' }}>
            Filament
          </h3>
          <div className="grid grid-cols-2 gap-3">
            <button className="h-12 rounded-xl text-sm font-medium transition-all active:scale-95 cursor-pointer flex items-center justify-center gap-2"
              style={{ background: 'var(--color-nozzle0)15', border: '1px solid var(--color-nozzle0)25', color: 'var(--color-nozzle0)', fontFamily: 'var(--font-display)' }}>
              <ArrowDown size={14} /> Load N0
            </button>
            <button className="h-12 rounded-xl text-sm font-medium transition-all active:scale-95 cursor-pointer flex items-center justify-center gap-2"
              style={{ background: 'var(--color-nozzle1)15', border: '1px solid var(--color-nozzle1)25', color: 'var(--color-nozzle1)', fontFamily: 'var(--font-display)' }}>
              <ArrowDown size={14} /> Load N1
            </button>
            <button className="h-12 rounded-xl text-sm font-medium transition-all active:scale-95 cursor-pointer flex items-center justify-center gap-2"
              style={{ background: 'var(--color-surface)', border: '1px solid var(--color-border)', color: 'var(--color-text-dim)', fontFamily: 'var(--font-display)' }}>
              <ArrowUp size={14} /> Unload N0
            </button>
            <button className="h-12 rounded-xl text-sm font-medium transition-all active:scale-95 cursor-pointer flex items-center justify-center gap-2"
              style={{ background: 'var(--color-surface)', border: '1px solid var(--color-border)', color: 'var(--color-text-dim)', fontFamily: 'var(--font-display)' }}>
              <ArrowUp size={14} /> Unload N1
            </button>
          </div>
        </div>

        {/* Manual temps */}
        <TempInput label="Nozzle 0" value={state.nozzle0Temp} color="var(--color-nozzle0)"
          onSet={t => controls.setNozzleTemp(0, t)} />
        <TempInput label="Nozzle 1" value={state.nozzle1Temp} color="var(--color-nozzle1)"
          onSet={t => controls.setNozzleTemp(1, t)} />
        <TempInput label="Heated Bed" value={state.bedTemp} color="#9e9e9e"
          onSet={t => controls.setBedTemp(t)} />
      </div>
    </div>
  );
}
