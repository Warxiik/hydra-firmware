import { Wifi, Monitor, Zap, Thermometer, Info, ChevronRight } from 'lucide-react';
import type { PrinterState } from '../hooks/usePrinter';

function SettingsCard({ icon: Icon, title, children, color = 'var(--color-accent)' }: {
  icon: typeof Wifi; title: string; children: React.ReactNode; color?: string;
}) {
  return (
    <div className="rounded-2xl p-5"
      style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
      <div className="flex items-center gap-2 mb-4">
        <Icon size={14} style={{ color }} />
        <h3 className="text-xs font-semibold tracking-widest uppercase"
          style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-display)' }}>
          {title}
        </h3>
      </div>
      {children}
    </div>
  );
}

function SettingsRow({ label, value, mono = true }: { label: string; value: string; mono?: boolean }) {
  return (
    <div className="flex items-center justify-between py-2.5 px-3 rounded-lg"
      style={{ background: 'var(--color-surface)' }}>
      <span className="text-xs" style={{ color: 'var(--color-text-dim)', fontFamily: 'var(--font-display)' }}>
        {label}
      </span>
      <span className="text-xs font-semibold"
        style={{ color: 'var(--color-text)', fontFamily: mono ? 'var(--font-mono)' : 'var(--font-display)' }}>
        {value}
      </span>
    </div>
  );
}

function SettingsLink({ label, onClick }: { label: string; onClick?: () => void }) {
  return (
    <button
      onClick={onClick}
      className="flex items-center justify-between py-3 px-3 rounded-lg w-full transition-all cursor-pointer text-left"
      style={{ background: 'var(--color-surface)' }}
      onMouseEnter={e => { e.currentTarget.style.background = 'var(--color-card-hover)'; }}
      onMouseLeave={e => { e.currentTarget.style.background = 'var(--color-surface)'; }}
    >
      <span className="text-xs" style={{ color: 'var(--color-text)', fontFamily: 'var(--font-display)' }}>
        {label}
      </span>
      <ChevronRight size={14} style={{ color: 'var(--color-text-muted)' }} />
    </button>
  );
}

export function SettingsPage({ state, onNavigate }: { state: PrinterState; onNavigate?: (page: 'network') => void }) {
  return (
    <div className="flex-1 overflow-y-auto p-5 animate-fade-in">
      <div className="max-w-[900px] mx-auto grid grid-cols-2 gap-4">
        <SettingsCard icon={Wifi} title="Network">
          <div className="flex flex-col gap-2">
            <SettingsRow label="WiFi SSID" value={state.wifiSSID} mono={false} />
            <SettingsRow label="IP Address" value={state.ipAddress} />
            <SettingsRow label="Status" value={state.wifiConnected ? 'Connected' : 'Disconnected'} mono={false} />
            <SettingsLink label="Configure WiFi" onClick={() => onNavigate?.('network')} />
          </div>
        </SettingsCard>

        <SettingsCard icon={Monitor} title="Display">
          <div className="flex flex-col gap-2">
            <SettingsRow label="Brightness" value="80%" />
            <SettingsRow label="Screen Timeout" value="5 min" />
            <SettingsRow label="Resolution" value="1280x720" />
            <SettingsLink label="Calibrate Touch" />
          </div>
        </SettingsCard>

        <SettingsCard icon={Zap} title="Motion">
          <div className="flex flex-col gap-2">
            <SettingsRow label="Max Velocity" value="300 mm/s" />
            <SettingsRow label="Max Acceleration" value="3000 mm/s²" />
            <SettingsRow label="Junction Deviation" value="0.05 mm" />
            <SettingsRow label="Steps/mm XY" value="80" />
            <SettingsRow label="Steps/mm Z" value="400" />
            <SettingsRow label="Steps/mm E" value="93" />
          </div>
        </SettingsCard>

        <SettingsCard icon={Thermometer} title="PID Tuning" color="var(--color-nozzle1)">
          <div className="flex flex-col gap-2">
            <div className="text-[10px] tracking-widest uppercase mb-1 px-1"
              style={{ color: 'var(--color-nozzle0)', fontFamily: 'var(--font-mono)' }}>Nozzle</div>
            <SettingsRow label="Kp" value="20.0" />
            <SettingsRow label="Ki" value="1.0" />
            <SettingsRow label="Kd" value="100.0" />
            <div className="text-[10px] tracking-widest uppercase mt-2 mb-1 px-1"
              style={{ color: 'var(--color-text-dim)', fontFamily: 'var(--font-mono)' }}>Bed</div>
            <SettingsRow label="Kp" value="10.0" />
            <SettingsRow label="Ki" value="0.5" />
            <SettingsRow label="Kd" value="200.0" />
            <SettingsLink label="Auto-tune PID" />
          </div>
        </SettingsCard>

        <SettingsCard icon={Info} title="About" color="var(--color-text-dim)">
          <div className="flex flex-col gap-2">
            <SettingsRow label="Firmware" value="v0.1.0" />
            <SettingsRow label="MCU" value="RP2040" mono={false} />
            <SettingsRow label="Host" value="Raspberry Pi CM5" mono={false} />
            <SettingsRow label="Build" value="2026-03-13" />
            <SettingsLink label="Check for Updates" />
            <SettingsLink label="Factory Reset" />
          </div>
        </SettingsCard>
      </div>
    </div>
  );
}
