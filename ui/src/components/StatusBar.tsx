import { Wifi, WifiOff, Flame, AlertTriangle } from 'lucide-react';
import type { PrinterState } from '../hooks/usePrinter';

const STATE_LABELS: Record<string, { text: string; color: string }> = {
  idle:     { text: 'IDLE',     color: 'var(--color-text-dim)' },
  homing:   { text: 'HOMING',  color: 'var(--color-accent)' },
  heating:  { text: 'HEATING', color: 'var(--color-warning)' },
  printing: { text: 'PRINTING', color: 'var(--color-success)' },
  paused:   { text: 'PAUSED',  color: 'var(--color-warning)' },
  error:    { text: 'ERROR',   color: 'var(--color-error)' },
};

function TempChip({ label, temp, target, color }: {
  label: string; temp: number; target: number; color: string;
}) {
  const isActive = target > 0;
  return (
    <div className="flex items-center gap-1.5 px-2.5 py-1 rounded-lg"
      style={{ background: isActive ? `${color}12` : 'transparent' }}>
      <Flame size={13} style={{ color: isActive ? color : 'var(--color-text-muted)' }} />
      <span className="text-xs" style={{
        fontFamily: 'var(--font-mono)',
        fontWeight: 600,
        color: isActive ? color : 'var(--color-text-dim)',
      }}>
        {label}
      </span>
      <span className="text-xs" style={{
        fontFamily: 'var(--font-mono)',
        fontWeight: 700,
        color: isActive ? 'var(--color-text)' : 'var(--color-text-dim)',
      }}>
        {temp.toFixed(0)}°
      </span>
      {target > 0 && (
        <span className="text-xs" style={{
          fontFamily: 'var(--font-mono)',
          color: 'var(--color-text-muted)',
        }}>
          /{target}°
        </span>
      )}
    </div>
  );
}

export function StatusBar({ state, connected = false }: { state: PrinterState; connected?: boolean }) {
  const stateInfo = STATE_LABELS[state.state] || STATE_LABELS.idle;

  return (
    <div className="flex items-center justify-between px-4 h-12 shrink-0"
      style={{
        background: 'var(--color-surface)',
        borderBottom: '1px solid var(--color-border-subtle)',
      }}>
      {/* Left: Logo + state */}
      <div className="flex items-center gap-3">
        <div className="flex items-center gap-1.5">
          <div className="w-5 h-5 rounded-md flex items-center justify-center"
            style={{ background: 'var(--color-accent)', boxShadow: '0 0 12px var(--color-accent-glow-strong)' }}>
            <span style={{ fontFamily: 'var(--font-mono)', fontWeight: 700, fontSize: 11, color: '#000' }}>H</span>
          </div>
          <span className="text-sm font-semibold tracking-wide" style={{ fontFamily: 'var(--font-display)' }}>
            HYDRA
          </span>
        </div>

        <div className="flex items-center gap-1.5 px-2 py-0.5 rounded-md"
          style={{ background: `${stateInfo.color}15`, border: `1px solid ${stateInfo.color}30` }}>
          {state.state === 'error' && <AlertTriangle size={12} style={{ color: stateInfo.color }} />}
          <span className="text-[10px] font-bold tracking-widest"
            style={{ color: stateInfo.color, fontFamily: 'var(--font-mono)' }}>
            {stateInfo.text}
          </span>
        </div>
      </div>

      {/* Center: Temps */}
      <div className="flex items-center gap-1">
        <TempChip label="N0" temp={state.nozzle0Temp} target={state.nozzle0Target} color="var(--color-nozzle0)" />
        <TempChip label="N1" temp={state.nozzle1Temp} target={state.nozzle1Target} color="var(--color-nozzle1)" />
        <TempChip label="BED" temp={state.bedTemp} target={state.bedTarget} color="var(--color-text-dim)" />
      </div>

      {/* Right: Connection + WiFi */}
      <div className="flex items-center gap-2">
        {!connected && (
          <span className="text-[9px] font-bold tracking-wider px-1.5 py-0.5 rounded"
            style={{
              color: 'var(--color-warning)',
              background: 'var(--color-warning)15',
              border: '1px solid var(--color-warning)30',
              fontFamily: 'var(--font-mono)',
            }}>
            DEMO
          </span>
        )}
        <span className="text-[10px]" style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-mono)' }}>
          {state.ipAddress}
        </span>
        {state.wifiConnected
          ? <Wifi size={16} style={{ color: 'var(--color-accent)' }} />
          : <WifiOff size={16} style={{ color: 'var(--color-text-muted)' }} />
        }
      </div>
    </div>
  );
}
