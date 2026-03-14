import { useState } from 'react';
import { AlertTriangle, Check, Loader2 } from 'lucide-react';

interface FilamentChangeDialogProps {
  nozzle: number;
  onConfirm: () => void;
}

export function FilamentChangeDialog({ nozzle, onConfirm }: FilamentChangeDialogProps) {
  const [step, setStep] = useState<'unload' | 'load' | 'priming'>('unload');

  const nozzleColor = nozzle === 0 ? 'var(--color-nozzle0)' : 'var(--color-nozzle1)';
  const nozzleLabel = `Nozzle ${nozzle}`;

  const handleLoadReady = () => {
    setStep('load');
  };

  const handleConfirm = () => {
    setStep('priming');
    onConfirm();
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center animate-fade-in"
      style={{ background: 'rgba(0, 0, 0, 0.7)', backdropFilter: 'blur(6px)' }}>
      <div className="rounded-3xl p-8 w-full max-w-[440px] mx-4"
        style={{
          background: 'var(--color-card)',
          border: '1px solid var(--color-border-subtle)',
          boxShadow: '0 24px 80px rgba(0, 0, 0, 0.5)',
        }}>

        {/* Header */}
        <div className="flex items-center gap-3 mb-6">
          <div className="w-10 h-10 rounded-xl flex items-center justify-center"
            style={{ background: 'var(--color-warning)20' }}>
            <AlertTriangle size={20} style={{ color: 'var(--color-warning)' }} />
          </div>
          <div>
            <h2 className="text-base font-bold"
              style={{ fontFamily: 'var(--font-display)', color: 'var(--color-text)' }}>
              Filament Change
            </h2>
            <p className="text-xs" style={{ color: nozzleColor, fontFamily: 'var(--font-mono)', fontWeight: 600 }}>
              {nozzleLabel}
            </p>
          </div>
        </div>

        {/* Steps indicator */}
        <div className="flex items-center gap-2 mb-6 px-1">
          {(['unload', 'load', 'priming'] as const).map((s, i) => {
            const labels = ['Unload', 'Load new', 'Prime'];
            const isCurrent = s === step;
            const isDone = ['unload', 'load', 'priming'].indexOf(step) > i;
            return (
              <div key={s} className="flex items-center gap-2 flex-1">
                <div className="w-5 h-5 rounded-full flex items-center justify-center shrink-0 text-[10px] font-bold"
                  style={{
                    background: isDone ? 'var(--color-accent)' : isCurrent ? 'var(--color-accent)20' : 'var(--color-surface)',
                    color: isDone ? '#000' : isCurrent ? 'var(--color-accent)' : 'var(--color-text-muted)',
                    border: isCurrent ? '1px solid var(--color-accent)' : '1px solid transparent',
                  }}>
                  {isDone ? <Check size={12} /> : i + 1}
                </div>
                <span className="text-[10px] tracking-wide"
                  style={{
                    color: isCurrent ? 'var(--color-text)' : 'var(--color-text-muted)',
                    fontFamily: 'var(--font-display)',
                    fontWeight: isCurrent ? 600 : 400,
                  }}>
                  {labels[i]}
                </span>
              </div>
            );
          })}
        </div>

        {/* Content */}
        <div className="rounded-xl p-5 mb-6" style={{ background: 'var(--color-surface)' }}>
          {step === 'unload' && (
            <div className="text-center">
              {/* Filament pull animation */}
              <div className="relative w-16 h-24 mx-auto mb-4">
                <div className="absolute bottom-0 left-1/2 -translate-x-1/2 w-6 h-8 rounded-b-lg"
                  style={{ background: 'var(--color-border)', borderRadius: '2px 2px 4px 4px' }} />
                <div className="absolute bottom-8 left-1/2 -translate-x-1/2 w-[3px] rounded-full animate-pulse"
                  style={{ background: nozzleColor, height: 40, opacity: 0.8 }} />
                <div className="absolute top-0 left-1/2 -translate-x-1/2 text-xs"
                  style={{ color: nozzleColor }}>
                  <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
                    <path d="M8 12V2M4 6l4-4 4 4" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" />
                  </svg>
                </div>
              </div>
              <p className="text-sm font-medium mb-1"
                style={{ color: 'var(--color-text)', fontFamily: 'var(--font-display)' }}>
                Filament retracted
              </p>
              <p className="text-xs" style={{ color: 'var(--color-text-dim)' }}>
                Remove the old filament from {nozzleLabel.toLowerCase()}, then press continue.
              </p>
            </div>
          )}

          {step === 'load' && (
            <div className="text-center">
              {/* Filament insert animation */}
              <div className="relative w-16 h-24 mx-auto mb-4">
                <div className="absolute bottom-0 left-1/2 -translate-x-1/2 w-6 h-8 rounded-b-lg"
                  style={{ background: 'var(--color-border)', borderRadius: '2px 2px 4px 4px' }} />
                <div className="absolute bottom-8 left-1/2 -translate-x-1/2 w-[3px] rounded-full animate-pulse"
                  style={{ background: nozzleColor, height: 20, opacity: 0.6 }} />
                <div className="absolute bottom-[4.5rem] left-1/2 -translate-x-1/2 text-xs"
                  style={{ color: nozzleColor }}>
                  <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
                    <path d="M8 4v10M4 10l4 4 4-4" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" />
                  </svg>
                </div>
              </div>
              <p className="text-sm font-medium mb-1"
                style={{ color: 'var(--color-text)', fontFamily: 'var(--font-display)' }}>
                Insert new filament
              </p>
              <p className="text-xs" style={{ color: 'var(--color-text-dim)' }}>
                Feed new filament into {nozzleLabel.toLowerCase()} until it reaches the hotend, then press confirm to prime.
              </p>
            </div>
          )}

          {step === 'priming' && (
            <div className="text-center">
              <Loader2 size={40} className="animate-spin mx-auto mb-4" style={{ color: nozzleColor }} />
              <p className="text-sm font-medium mb-1"
                style={{ color: 'var(--color-text)', fontFamily: 'var(--font-display)' }}>
                Priming extruder...
              </p>
              <p className="text-xs" style={{ color: 'var(--color-text-dim)' }}>
                Extruding 30mm of filament to purge the nozzle. Print will resume automatically.
              </p>
            </div>
          )}
        </div>

        {/* Action button */}
        {step === 'unload' && (
          <button
            onClick={handleLoadReady}
            className="w-full h-14 rounded-xl font-semibold text-sm transition-all active:scale-[0.98] cursor-pointer"
            style={{
              background: 'var(--color-accent)',
              color: '#000',
              fontFamily: 'var(--font-display)',
              border: 'none',
            }}>
            Old filament removed — Continue
          </button>
        )}

        {step === 'load' && (
          <button
            onClick={handleConfirm}
            className="w-full h-14 rounded-xl font-semibold text-sm transition-all active:scale-[0.98] cursor-pointer"
            style={{
              background: 'var(--color-success)',
              color: '#000',
              fontFamily: 'var(--font-display)',
              border: 'none',
            }}>
            <span className="flex items-center justify-center gap-2">
              <Check size={18} /> New filament loaded — Confirm
            </span>
          </button>
        )}

        {step === 'priming' && (
          <div className="w-full h-14 rounded-xl flex items-center justify-center text-sm"
            style={{
              background: 'var(--color-surface)',
              color: 'var(--color-text-muted)',
              fontFamily: 'var(--font-display)',
            }}>
            Please wait...
          </div>
        )}
      </div>
    </div>
  );
}
