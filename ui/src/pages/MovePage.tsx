import { useState } from 'react';
import { ArrowUp, ArrowDown, ArrowLeft, ArrowRight, Home, ChevronUp, ChevronDown, Crosshair } from 'lucide-react';
import type { PrinterControls } from '../hooks/usePrinter';

const STEP_SIZES = [0.1, 1, 10, 100];

function JogButton({ icon: Icon, onClick, size = 52, accent = false }: {
  icon: typeof ArrowUp; onClick: () => void; size?: number; accent?: boolean;
}) {
  return (
    <button
      onClick={onClick}
      className="flex items-center justify-center rounded-xl transition-all active:scale-90 cursor-pointer"
      style={{
        width: size, height: size,
        background: accent ? 'var(--color-accent)18' : 'var(--color-surface)',
        border: `1px solid ${accent ? 'var(--color-accent)35' : 'var(--color-border)'}`,
      }}
      onMouseEnter={e => { e.currentTarget.style.borderColor = 'var(--color-accent)'; }}
      onMouseLeave={e => { e.currentTarget.style.borderColor = accent ? 'var(--color-accent)35' : 'var(--color-border)'; }}
    >
      <Icon size={20} style={{ color: accent ? 'var(--color-accent)' : 'var(--color-text)' }} />
    </button>
  );
}

export function MovePage({ controls }: { controls: PrinterControls }) {
  const [stepSize, setStepSize] = useState(10);

  return (
    <div className="flex-1 overflow-y-auto p-5 animate-fade-in">
      <div className="max-w-[900px] mx-auto flex gap-6 items-start justify-center">
        {/* XY Jog Pad */}
        <div className="rounded-2xl p-6"
          style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
          <h3 className="text-xs font-semibold tracking-widest uppercase mb-5 text-center"
            style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-display)' }}>
            XY Movement
          </h3>

          <div className="grid grid-cols-3 gap-2 w-fit mx-auto" style={{ gridTemplateRows: 'repeat(3, auto)' }}>
            {/* Row 1: empty, up, empty */}
            <div />
            <JogButton icon={ArrowUp} onClick={() => controls.jog('Y', stepSize)} />
            <div />

            {/* Row 2: left, home, right */}
            <JogButton icon={ArrowLeft} onClick={() => controls.jog('X', -stepSize)} />
            <JogButton icon={Crosshair} onClick={controls.homeAll} accent />
            <JogButton icon={ArrowRight} onClick={() => controls.jog('X', stepSize)} />

            {/* Row 3: empty, down, empty */}
            <div />
            <JogButton icon={ArrowDown} onClick={() => controls.jog('Y', -stepSize)} />
            <div />
          </div>

          {/* Home individual axes */}
          <div className="flex gap-2 mt-5 justify-center">
            {['X', 'Y'].map(axis => (
              <button key={axis}
                onClick={() => controls.homeAxis(axis as 'X' | 'Y')}
                className="flex items-center gap-1.5 px-4 h-10 rounded-lg text-xs font-semibold transition-all active:scale-95 cursor-pointer"
                style={{
                  background: 'var(--color-surface)',
                  border: '1px solid var(--color-border)',
                  fontFamily: 'var(--font-mono)',
                  color: 'var(--color-text-dim)',
                }}>
                <Home size={12} /> {axis}
              </button>
            ))}
          </div>
        </div>

        {/* Z Jog */}
        <div className="rounded-2xl p-6"
          style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
          <h3 className="text-xs font-semibold tracking-widest uppercase mb-5 text-center"
            style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-display)' }}>
            Z Axis
          </h3>

          <div className="flex flex-col items-center gap-2">
            <JogButton icon={ChevronUp} onClick={() => controls.jog('Z', stepSize)} size={60} />

            <div className="py-3 px-4 rounded-lg text-center" style={{ background: 'var(--color-surface)' }}>
              <span className="text-[10px] tracking-widest block" style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-mono)' }}>
                Z
              </span>
              <span className="text-lg font-bold" style={{ fontFamily: 'var(--font-mono)' }}>
                0.00
              </span>
              <span className="text-[10px] ml-0.5" style={{ color: 'var(--color-text-dim)' }}>mm</span>
            </div>

            <JogButton icon={ChevronDown} onClick={() => controls.jog('Z', -stepSize)} size={60} />
          </div>

          <button
            onClick={() => controls.homeAxis('Z')}
            className="flex items-center gap-1.5 px-4 h-10 rounded-lg text-xs font-semibold transition-all active:scale-95 cursor-pointer mt-5 w-full justify-center"
            style={{
              background: 'var(--color-surface)',
              border: '1px solid var(--color-border)',
              fontFamily: 'var(--font-mono)',
              color: 'var(--color-text-dim)',
            }}>
            <Home size={12} /> Home Z
          </button>
        </div>

        {/* Step size + extruder */}
        <div className="flex flex-col gap-5">
          {/* Step size */}
          <div className="rounded-2xl p-5"
            style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
            <h3 className="text-xs font-semibold tracking-widest uppercase mb-4"
              style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-display)' }}>
              Step Size
            </h3>

            <div className="flex flex-col gap-2">
              {STEP_SIZES.map(s => (
                <button key={s}
                  onClick={() => setStepSize(s)}
                  className="h-12 rounded-xl text-sm font-semibold transition-all active:scale-95 cursor-pointer px-5 text-left"
                  style={{
                    background: s === stepSize ? 'var(--color-accent)18' : 'var(--color-surface)',
                    border: `1px solid ${s === stepSize ? 'var(--color-accent)50' : 'var(--color-border)'}`,
                    color: s === stepSize ? 'var(--color-accent)' : 'var(--color-text-dim)',
                    fontFamily: 'var(--font-mono)',
                  }}>
                  {s} mm
                </button>
              ))}
            </div>
          </div>

          {/* Extruder jog */}
          <div className="rounded-2xl p-5"
            style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
            <h3 className="text-xs font-semibold tracking-widest uppercase mb-3"
              style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-display)' }}>
              Extruder
            </h3>

            <div className="grid grid-cols-2 gap-2">
              <button onClick={() => controls.jog('E', 5)}
                className="h-11 rounded-lg text-xs font-medium flex items-center justify-center gap-1 transition-all active:scale-95 cursor-pointer"
                style={{ background: 'var(--color-nozzle0)12', border: '1px solid var(--color-nozzle0)25', color: 'var(--color-nozzle0)', fontFamily: 'var(--font-mono)' }}>
                <ArrowDown size={12} /> E0 +5
              </button>
              <button onClick={() => controls.jog('E', -5)}
                className="h-11 rounded-lg text-xs font-medium flex items-center justify-center gap-1 transition-all active:scale-95 cursor-pointer"
                style={{ background: 'var(--color-surface)', border: '1px solid var(--color-border)', color: 'var(--color-text-dim)', fontFamily: 'var(--font-mono)' }}>
                <ArrowUp size={12} /> E0 -5
              </button>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
