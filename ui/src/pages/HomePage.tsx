import { Home, Flame, ArrowUpFromDot, Crosshair } from 'lucide-react';
import { TempGauge } from '../components/TempGauge';
import type { PrinterState, PrinterControls } from '../hooks/usePrinter';

const PRESETS = [
  { name: 'PLA', nozzle: 210, bed: 60, color: 'var(--color-accent)' },
  { name: 'PETG', nozzle: 235, bed: 80, color: 'var(--color-nozzle0)' },
  { name: 'ABS', nozzle: 245, bed: 100, color: 'var(--color-nozzle1)' },
];

function ActionButton({ icon: Icon, label, onClick, color = 'var(--color-accent)' }: {
  icon: typeof Home; label: string; onClick: () => void; color?: string;
}) {
  return (
    <button
      onClick={onClick}
      className="flex flex-col items-center justify-center gap-2 rounded-xl p-4 min-h-[80px] transition-all duration-200 active:scale-95 cursor-pointer"
      style={{
        background: `${color}08`,
        border: `1px solid ${color}20`,
      }}
      onMouseEnter={e => {
        e.currentTarget.style.background = `${color}18`;
        e.currentTarget.style.borderColor = `${color}40`;
      }}
      onMouseLeave={e => {
        e.currentTarget.style.background = `${color}08`;
        e.currentTarget.style.borderColor = `${color}20`;
      }}
    >
      <Icon size={22} style={{ color }} />
      <span className="text-xs font-medium" style={{ color: 'var(--color-text-dim)', fontFamily: 'var(--font-display)' }}>
        {label}
      </span>
    </button>
  );
}

export function HomePage({ state, controls }: { state: PrinterState; controls: PrinterControls }) {
  return (
    <div className="flex-1 overflow-y-auto p-5 animate-fade-in">
      <div className="max-w-[1100px] mx-auto grid grid-cols-[1fr_auto] gap-5 h-full">
        {/* Left column: Temps */}
        <div className="flex flex-col gap-5">
          {/* Temperature gauges row */}
          <div className="rounded-2xl p-5 flex items-center justify-around"
            style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
            <TempGauge label="Nozzle 0" current={state.nozzle0Temp} target={state.nozzle0Target} color="var(--color-nozzle0)" size={160} />
            <TempGauge label="Nozzle 1" current={state.nozzle1Temp} target={state.nozzle1Target} color="var(--color-nozzle1)" size={160} />
            <TempGauge label="Bed" current={state.bedTemp} target={state.bedTarget} color="#9e9e9e" size={140} maxTemp={120} />
          </div>

          {/* Printer status card */}
          <div className="rounded-2xl p-5 flex-1"
            style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
            <div className="flex items-center gap-3 mb-4">
              <div className="w-2 h-2 rounded-full" style={{
                background: state.state === 'idle' ? 'var(--color-accent)' : 'var(--color-success)',
                boxShadow: `0 0 8px ${state.state === 'idle' ? 'var(--color-accent-glow-strong)' : 'rgba(76,175,80,0.4)'}`,
              }} />
              <h3 className="text-sm font-semibold tracking-wide" style={{ fontFamily: 'var(--font-display)' }}>
                Printer Status
              </h3>
            </div>

            <div className="grid grid-cols-2 gap-3">
              <div className="rounded-lg p-3" style={{ background: 'var(--color-surface)' }}>
                <span className="text-[10px] uppercase tracking-widest" style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-mono)' }}>State</span>
                <p className="text-sm font-semibold mt-1" style={{ fontFamily: 'var(--font-mono)', color: 'var(--color-accent)' }}>
                  {state.state.toUpperCase()}
                </p>
              </div>
              <div className="rounded-lg p-3" style={{ background: 'var(--color-surface)' }}>
                <span className="text-[10px] uppercase tracking-widest" style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-mono)' }}>WiFi</span>
                <p className="text-sm font-semibold mt-1" style={{ fontFamily: 'var(--font-mono)' }}>
                  {state.wifiSSID}
                </p>
              </div>
            </div>
          </div>
        </div>

        {/* Right column: Quick actions */}
        <div className="w-[220px] flex flex-col gap-3">
          <h3 className="text-xs font-semibold tracking-widest uppercase px-1"
            style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-display)' }}>
            Quick Actions
          </h3>

          {/* Preheat presets */}
          {PRESETS.map(preset => (
            <ActionButton
              key={preset.name}
              icon={Flame}
              label={`Preheat ${preset.name}`}
              color={preset.color}
              onClick={() => {
                controls.setNozzleTemp(0, preset.nozzle);
                controls.setNozzleTemp(1, preset.nozzle);
                controls.setBedTemp(preset.bed);
              }}
            />
          ))}

          <div className="h-px my-1" style={{ background: 'var(--color-border-subtle)' }} />

          <ActionButton icon={Crosshair} label="Home All" color="var(--color-accent)" onClick={controls.homeAll} />

          <ActionButton
            icon={ArrowUpFromDot}
            label="Cooldown"
            color="var(--color-text-dim)"
            onClick={() => {
              controls.setNozzleTemp(0, 0);
              controls.setNozzleTemp(1, 0);
              controls.setBedTemp(0);
            }}
          />
        </div>
      </div>
    </div>
  );
}
