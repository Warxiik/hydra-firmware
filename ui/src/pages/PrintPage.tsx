import { Pause, Play, X, Gauge, Droplets, Wind, Layers } from 'lucide-react';
import { LineChart, Line, XAxis, YAxis, ResponsiveContainer, Tooltip } from 'recharts';
import type { PrinterState, PrinterControls } from '../hooks/usePrinter';

function formatTime(seconds: number): string {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  if (h > 0) return `${h}h ${m.toString().padStart(2, '0')}m`;
  return `${m}m ${s.toString().padStart(2, '0')}s`;
}

function OverrideSlider({ icon: Icon, label, value, unit, min, max, onChange, color = 'var(--color-accent)' }: {
  icon: typeof Gauge; label: string; value: number; unit: string;
  min: number; max: number; onChange: (v: number) => void; color?: string;
}) {
  return (
    <div className="flex items-center gap-3 px-3 py-2 rounded-lg" style={{ background: 'var(--color-surface)' }}>
      <Icon size={16} style={{ color }} />
      <span className="text-xs w-10" style={{ color: 'var(--color-text-dim)', fontFamily: 'var(--font-display)' }}>
        {label}
      </span>
      <input
        type="range" min={min} max={max} value={value}
        onChange={e => onChange(Number(e.target.value))}
        className="flex-1 h-1 rounded-full appearance-none cursor-pointer"
        style={{
          background: `linear-gradient(to right, ${color} 0%, ${color} ${((value - min) / (max - min)) * 100}%, var(--color-border) ${((value - min) / (max - min)) * 100}%, var(--color-border) 100%)`,
          accentColor: color,
        }}
      />
      <span className="text-xs w-12 text-right" style={{ fontFamily: 'var(--font-mono)', fontWeight: 600, color: 'var(--color-text)' }}>
        {value}{unit}
      </span>
    </div>
  );
}

function ProgressRing({ progress, size = 200 }: { progress: number; size?: number }) {
  const radius = (size - 20) / 2;
  const cx = size / 2;
  const cy = size / 2;
  const circumference = 2 * Math.PI * radius;
  const dash = progress * circumference;
  const pct = Math.round(progress * 100);

  return (
    <svg width={size} height={size} viewBox={`0 0 ${size} ${size}`}>
      <defs>
        <linearGradient id="progress-grad" x1="0%" y1="0%" x2="100%" y2="100%">
          <stop offset="0%" stopColor="var(--color-accent-bright)" />
          <stop offset="100%" stopColor="var(--color-accent-dim)" />
        </linearGradient>
        <filter id="progress-glow">
          <feGaussianBlur stdDeviation="6" result="blur" />
          <feMerge>
            <feMergeNode in="blur" />
            <feMergeNode in="SourceGraphic" />
          </feMerge>
        </filter>
      </defs>

      <circle cx={cx} cy={cy} r={radius} fill="none" stroke="var(--color-border)" strokeWidth={8} opacity={0.4} />

      <circle
        cx={cx} cy={cy} r={radius}
        fill="none"
        stroke="url(#progress-grad)"
        strokeWidth={8}
        strokeDasharray={`${dash} ${circumference}`}
        strokeLinecap="round"
        transform={`rotate(-90 ${cx} ${cy})`}
        filter="url(#progress-glow)"
        style={{ transition: 'stroke-dasharray 1s ease-out' }}
      />

      <text x={cx} y={cy - 10} textAnchor="middle" dominantBaseline="central"
        fill="var(--color-text)" fontFamily="var(--font-mono)" fontWeight={700} fontSize={42}>
        {pct}
      </text>
      <text x={cx} y={cy + 22} textAnchor="middle" dominantBaseline="central"
        fill="var(--color-text-dim)" fontFamily="var(--font-mono)" fontSize={14}>
        percent
      </text>
    </svg>
  );
}

export function PrintPage({ state, controls }: { state: PrinterState; controls: PrinterControls }) {
  const isPrinting = state.state === 'printing';
  const isPaused = state.state === 'paused';

  return (
    <div className="flex-1 overflow-y-auto p-5 animate-fade-in">
      <div className="max-w-[1100px] mx-auto grid grid-cols-[1fr_320px] gap-5 h-full">
        {/* Left: Progress + chart */}
        <div className="flex flex-col gap-4">
          {/* Progress + stats */}
          <div className="rounded-2xl p-5 flex items-center gap-8"
            style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
            <ProgressRing progress={state.progress} size={180} />

            <div className="flex-1 grid grid-cols-2 gap-3">
              {[
                { label: 'File', value: state.fileName || '—', mono: false },
                { label: 'Layer', value: `${state.currentLayer} / ${state.totalLayers}`, mono: true },
                { label: 'Elapsed', value: formatTime(state.elapsedSeconds), mono: true },
                { label: 'Remaining', value: formatTime(state.remainingSeconds), mono: true },
              ].map(item => (
                <div key={item.label} className="rounded-lg p-3" style={{ background: 'var(--color-surface)' }}>
                  <span className="text-[10px] uppercase tracking-widest block"
                    style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-mono)' }}>
                    {item.label}
                  </span>
                  <p className="text-sm font-semibold mt-1 truncate"
                    style={{ fontFamily: item.mono ? 'var(--font-mono)' : 'var(--font-display)' }}>
                    {item.value}
                  </p>
                </div>
              ))}
            </div>
          </div>

          {/* Temperature chart */}
          <div className="rounded-2xl p-4 flex-1 min-h-[160px]"
            style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
            <div className="flex items-center gap-2 mb-2">
              <Layers size={14} style={{ color: 'var(--color-text-muted)' }} />
              <span className="text-xs font-medium tracking-wide" style={{ color: 'var(--color-text-dim)' }}>
                Temperature History
              </span>
              <div className="flex items-center gap-3 ml-auto">
                <span className="flex items-center gap-1 text-[10px]" style={{ color: 'var(--color-nozzle0)', fontFamily: 'var(--font-mono)' }}>
                  <span className="w-2 h-2 rounded-full" style={{ background: 'var(--color-nozzle0)' }} /> N0
                </span>
                <span className="flex items-center gap-1 text-[10px]" style={{ color: 'var(--color-nozzle1)', fontFamily: 'var(--font-mono)' }}>
                  <span className="w-2 h-2 rounded-full" style={{ background: 'var(--color-nozzle1)' }} /> N1
                </span>
                <span className="flex items-center gap-1 text-[10px]" style={{ color: 'var(--color-text-dim)', fontFamily: 'var(--font-mono)' }}>
                  <span className="w-2 h-2 rounded-full" style={{ background: '#9e9e9e' }} /> Bed
                </span>
              </div>
            </div>
            <ResponsiveContainer width="100%" height="85%">
              <LineChart data={state.tempHistory}>
                <XAxis dataKey="time" hide />
                <YAxis domain={[0, 280]} hide />
                <Tooltip
                  contentStyle={{
                    background: 'var(--color-card)',
                    border: '1px solid var(--color-border)',
                    borderRadius: 8,
                    fontFamily: 'var(--font-mono)',
                    fontSize: 11,
                  }}
                  labelStyle={{ display: 'none' }}
                />
                <Line type="monotone" dataKey="n0" stroke="var(--color-nozzle0)" strokeWidth={2} dot={false} />
                <Line type="monotone" dataKey="n1" stroke="var(--color-nozzle1)" strokeWidth={2} dot={false} />
                <Line type="monotone" dataKey="bed" stroke="#9e9e9e" strokeWidth={1.5} dot={false} />
              </LineChart>
            </ResponsiveContainer>
          </div>
        </div>

        {/* Right: Controls */}
        <div className="flex flex-col gap-3">
          {/* Overrides */}
          <div className="rounded-2xl p-4 flex flex-col gap-2"
            style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
            <h4 className="text-xs font-semibold tracking-widest uppercase mb-1"
              style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-display)' }}>
              Overrides
            </h4>
            <OverrideSlider icon={Gauge} label="Speed" value={state.speedOverride} unit="%" min={10} max={300}
              onChange={controls.setSpeedOverride} />
            <OverrideSlider icon={Droplets} label="Flow" value={state.flowOverride} unit="%" min={50} max={150}
              onChange={controls.setFlowOverride} color="var(--color-nozzle0)" />
            <OverrideSlider icon={Wind} label="Fan" value={state.fanSpeed} unit="%" min={0} max={100}
              onChange={controls.setFanSpeed} color="var(--color-text-dim)" />
          </div>

          {/* Nozzle temps compact */}
          <div className="rounded-2xl p-4 grid grid-cols-3 gap-3"
            style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
            {[
              { label: 'N0', temp: state.nozzle0Temp, target: state.nozzle0Target, color: 'var(--color-nozzle0)' },
              { label: 'N1', temp: state.nozzle1Temp, target: state.nozzle1Target, color: 'var(--color-nozzle1)' },
              { label: 'BED', temp: state.bedTemp, target: state.bedTarget, color: '#9e9e9e' },
            ].map(t => (
              <div key={t.label} className="text-center">
                <span className="text-[10px] tracking-widest block"
                  style={{ color: t.color, fontFamily: 'var(--font-mono)', fontWeight: 600 }}>{t.label}</span>
                <span className="text-lg block" style={{ fontFamily: 'var(--font-mono)', fontWeight: 700 }}>
                  {t.temp.toFixed(0)}°
                </span>
                {t.target > 0 && (
                  <span className="text-[10px]" style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-mono)' }}>
                    /{t.target}°
                  </span>
                )}
              </div>
            ))}
          </div>

          {/* Spacer */}
          <div className="flex-1" />

          {/* Print controls */}
          <div className="flex gap-3">
            {isPrinting && (
              <button onClick={controls.pausePrint}
                className="flex-1 flex items-center justify-center gap-2 h-14 rounded-xl font-semibold text-sm transition-all active:scale-95 cursor-pointer"
                style={{
                  background: 'var(--color-warning)',
                  color: '#000',
                  fontFamily: 'var(--font-display)',
                }}>
                <Pause size={18} /> Pause
              </button>
            )}
            {isPaused && (
              <button onClick={controls.resumePrint}
                className="flex-1 flex items-center justify-center gap-2 h-14 rounded-xl font-semibold text-sm transition-all active:scale-95 cursor-pointer"
                style={{
                  background: 'var(--color-success)',
                  color: '#000',
                  fontFamily: 'var(--font-display)',
                }}>
                <Play size={18} /> Resume
              </button>
            )}
            {(isPrinting || isPaused) && (
              <button onClick={controls.cancelPrint}
                className="flex items-center justify-center gap-2 h-14 px-6 rounded-xl font-semibold text-sm transition-all active:scale-95 cursor-pointer"
                style={{
                  background: 'var(--color-error)18',
                  border: '1px solid var(--color-error)40',
                  color: 'var(--color-error)',
                  fontFamily: 'var(--font-display)',
                }}>
                <X size={18} /> Cancel
              </button>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}
