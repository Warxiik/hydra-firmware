import { useState } from 'react';
import { Wifi, WifiOff, Lock, RefreshCw, Check, ArrowLeft } from 'lucide-react';
import type { PrinterState } from '../hooks/usePrinter';

interface WifiNetwork {
  ssid: string;
  signal: number;
  secured: boolean;
  connected: boolean;
}

const MOCK_NETWORKS: WifiNetwork[] = [
  { ssid: 'Workshop-5G', signal: 90, secured: true, connected: true },
  { ssid: 'Makers_Lab', signal: 72, secured: true, connected: false },
  { ssid: 'Guest_Network', signal: 45, secured: false, connected: false },
  { ssid: 'IoT-Devices', signal: 38, secured: true, connected: false },
];

function SignalIcon({ strength }: { strength: number }) {
  const bars = strength > 75 ? 3 : strength > 50 ? 2 : 1;
  return (
    <div className="flex items-end gap-[2px]" style={{ height: 14 }}>
      {[1, 2, 3].map(i => (
        <div
          key={i}
          className="rounded-sm"
          style={{
            width: 3,
            height: 4 + i * 3,
            background: i <= bars ? 'var(--color-accent)' : 'var(--color-border-subtle)',
            transition: 'background 0.2s',
          }}
        />
      ))}
    </div>
  );
}

function NetworkCard({ network, onConnect }: {
  network: WifiNetwork;
  onConnect: (ssid: string) => void;
}) {
  return (
    <button
      onClick={() => !network.connected && onConnect(network.ssid)}
      className="flex items-center justify-between py-3 px-4 rounded-xl w-full transition-all cursor-pointer text-left"
      style={{
        background: network.connected ? 'var(--color-accent-bg)' : 'var(--color-surface)',
        border: network.connected ? '1px solid var(--color-accent)' : '1px solid transparent',
      }}
      onMouseEnter={e => {
        if (!network.connected) e.currentTarget.style.background = 'var(--color-card-hover)';
      }}
      onMouseLeave={e => {
        if (!network.connected) e.currentTarget.style.background = 'var(--color-surface)';
      }}
    >
      <div className="flex items-center gap-3">
        <Wifi size={16} style={{
          color: network.connected ? 'var(--color-accent)' : 'var(--color-text-muted)',
        }} />
        <div>
          <div className="text-sm font-medium" style={{
            color: network.connected ? 'var(--color-accent)' : 'var(--color-text)',
            fontFamily: 'var(--font-display)',
          }}>
            {network.ssid}
          </div>
          {network.connected && (
            <div className="text-[10px]" style={{ color: 'var(--color-accent)', fontFamily: 'var(--font-mono)' }}>
              Connected
            </div>
          )}
        </div>
      </div>
      <div className="flex items-center gap-2">
        {network.secured && <Lock size={12} style={{ color: 'var(--color-text-dim)' }} />}
        <SignalIcon strength={network.signal} />
        {network.connected && <Check size={14} style={{ color: 'var(--color-accent)' }} />}
      </div>
    </button>
  );
}

export function NetworkPage({ state, onBack }: { state: PrinterState; onBack?: () => void }) {
  const [scanning, setScanning] = useState(false);
  const [networks] = useState<WifiNetwork[]>(MOCK_NETWORKS);

  const handleScan = () => {
    setScanning(true);
    setTimeout(() => setScanning(false), 2000);
  };

  const handleConnect = (ssid: string) => {
    console.log('Connect to:', ssid);
  };

  return (
    <div className="flex-1 overflow-y-auto p-5 animate-fade-in">
      <div className="max-w-[600px] mx-auto">
        {/* Back button */}
        {onBack && (
          <button
            onClick={onBack}
            className="flex items-center gap-2 mb-4 px-1 text-xs font-medium cursor-pointer"
            style={{ color: 'var(--color-accent)', fontFamily: 'var(--font-display)', background: 'none', border: 'none' }}
          >
            <ArrowLeft size={14} /> Back to Settings
          </button>
        )}

        {/* Status card */}
        <div className="rounded-2xl p-5 mb-4"
          style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
          <div className="flex items-center justify-between mb-4">
            <div className="flex items-center gap-2">
              {state.wifiConnected
                ? <Wifi size={16} style={{ color: 'var(--color-accent)' }} />
                : <WifiOff size={16} style={{ color: 'var(--color-danger)' }} />
              }
              <h3 className="text-xs font-semibold tracking-widest uppercase"
                style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-display)' }}>
                Connection Status
              </h3>
            </div>
            <button
              onClick={handleScan}
              className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-[11px] font-medium cursor-pointer"
              style={{
                background: 'var(--color-surface)',
                color: 'var(--color-accent)',
                fontFamily: 'var(--font-display)',
                border: 'none',
              }}
            >
              <RefreshCw size={12} className={scanning ? 'animate-spin' : ''} />
              Scan
            </button>
          </div>

          <div className="flex flex-col gap-2">
            <div className="flex items-center justify-between py-2 px-3 rounded-lg"
              style={{ background: 'var(--color-surface)' }}>
              <span className="text-xs" style={{ color: 'var(--color-text-dim)' }}>SSID</span>
              <span className="text-xs font-semibold" style={{ color: 'var(--color-text)' }}>
                {state.wifiSSID || 'Not connected'}
              </span>
            </div>
            <div className="flex items-center justify-between py-2 px-3 rounded-lg"
              style={{ background: 'var(--color-surface)' }}>
              <span className="text-xs" style={{ color: 'var(--color-text-dim)' }}>IP Address</span>
              <span className="text-xs font-semibold" style={{
                color: 'var(--color-text)',
                fontFamily: 'var(--font-mono)',
              }}>
                {state.ipAddress || '—'}
              </span>
            </div>
            <div className="flex items-center justify-between py-2 px-3 rounded-lg"
              style={{ background: 'var(--color-surface)' }}>
              <span className="text-xs" style={{ color: 'var(--color-text-dim)' }}>Signal</span>
              <SignalIcon strength={state.wifiConnected ? 90 : 0} />
            </div>
          </div>
        </div>

        {/* Available networks */}
        <div className="rounded-2xl p-5"
          style={{ background: 'var(--color-card)', border: '1px solid var(--color-border-subtle)' }}>
          <h3 className="text-xs font-semibold tracking-widest uppercase mb-4"
            style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-display)' }}>
            Available Networks
          </h3>
          <div className="flex flex-col gap-2">
            {networks.map(network => (
              <NetworkCard
                key={network.ssid}
                network={network}
                onConnect={handleConnect}
              />
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}
