import { useState } from 'react';
import { File, Upload, Play, Trash2, Clock, HardDrive } from 'lucide-react';
import type { PrinterControls } from '../hooks/usePrinter';

interface GcodeFile {
  name: string;
  size: string;
  date: string;
  layers: number;
  estimatedTime: string;
}

const MOCK_FILES: GcodeFile[] = [
  { name: 'dual_towers_40+30mm', size: '2.4 MB', date: '2026-03-12', layers: 200, estimatedTime: '1h 40m' },
  { name: 'calibration_cube_dual', size: '812 KB', date: '2026-03-11', layers: 100, estimatedTime: '45m' },
  { name: 'benchy_dual_nozzle', size: '3.1 MB', date: '2026-03-10', layers: 312, estimatedTime: '2h 10m' },
  { name: 'phone_stand_v3', size: '5.8 MB', date: '2026-03-09', layers: 450, estimatedTime: '3h 20m' },
  { name: 'vase_spiral_test', size: '1.2 MB', date: '2026-03-08', layers: 280, estimatedTime: '1h 15m' },
  { name: 'gear_set_12t_20t', size: '4.5 MB', date: '2026-03-07', layers: 180, estimatedTime: '2h 45m' },
];

export function FilesPage({ controls }: { controls: PrinterControls }) {
  const [selected, setSelected] = useState<string | null>(null);
  const files = MOCK_FILES;

  return (
    <div className="flex-1 overflow-y-auto p-5 animate-fade-in">
      <div className="max-w-[900px] mx-auto">
        {/* Header */}
        <div className="flex items-center justify-between mb-4">
          <div className="flex items-center gap-2">
            <HardDrive size={16} style={{ color: 'var(--color-accent)' }} />
            <h2 className="text-sm font-semibold tracking-wide" style={{ fontFamily: 'var(--font-display)' }}>
              G-Code Files
            </h2>
            <span className="text-xs px-2 py-0.5 rounded-md"
              style={{ background: 'var(--color-surface)', color: 'var(--color-text-muted)', fontFamily: 'var(--font-mono)' }}>
              {files.length}
            </span>
          </div>

          <button className="flex items-center gap-2 h-10 px-4 rounded-xl text-xs font-semibold transition-all active:scale-95 cursor-pointer"
            style={{
              background: 'var(--color-accent)15',
              border: '1px solid var(--color-accent)30',
              color: 'var(--color-accent)',
              fontFamily: 'var(--font-display)',
            }}>
            <Upload size={14} /> Upload
          </button>
        </div>

        {/* File list */}
        <div className="flex flex-col gap-2">
          {files.map(file => {
            const isSelected = selected === file.name;
            return (
              <button key={file.name}
                onClick={() => setSelected(isSelected ? null : file.name)}
                className="flex items-center gap-4 p-4 rounded-xl transition-all active:scale-[0.99] cursor-pointer text-left w-full"
                style={{
                  background: isSelected ? 'var(--color-accent)10' : 'var(--color-card)',
                  border: `1px solid ${isSelected ? 'var(--color-accent)40' : 'var(--color-border-subtle)'}`,
                }}>
                <div className="w-10 h-10 rounded-lg flex items-center justify-center shrink-0"
                  style={{ background: isSelected ? 'var(--color-accent)20' : 'var(--color-surface)' }}>
                  <File size={18} style={{ color: isSelected ? 'var(--color-accent)' : 'var(--color-text-dim)' }} />
                </div>

                <div className="flex-1 min-w-0">
                  <span className="text-sm font-semibold block truncate"
                    style={{ fontFamily: 'var(--font-display)', color: isSelected ? 'var(--color-accent)' : 'var(--color-text)' }}>
                    {file.name}
                  </span>
                  <div className="flex items-center gap-3 mt-1">
                    <span className="text-[10px]" style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-mono)' }}>
                      {file.size}
                    </span>
                    <span className="text-[10px]" style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-mono)' }}>
                      {file.layers} layers
                    </span>
                    <span className="flex items-center gap-1 text-[10px]" style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-mono)' }}>
                      <Clock size={9} /> {file.estimatedTime}
                    </span>
                  </div>
                </div>

                <span className="text-[10px] shrink-0"
                  style={{ color: 'var(--color-text-muted)', fontFamily: 'var(--font-mono)' }}>
                  {file.date}
                </span>

                {isSelected && (
                  <div className="flex gap-2 shrink-0" onClick={e => e.stopPropagation()}>
                    <button
                      onClick={() => controls.startPrint(file.name)}
                      className="flex items-center gap-1.5 h-10 px-4 rounded-lg text-xs font-semibold transition-all active:scale-95 cursor-pointer"
                      style={{ background: 'var(--color-success)', color: '#000', fontFamily: 'var(--font-display)' }}>
                      <Play size={14} /> Print
                    </button>
                    <button className="flex items-center justify-center w-10 h-10 rounded-lg transition-all active:scale-95 cursor-pointer"
                      style={{ background: 'var(--color-error)15', border: '1px solid var(--color-error)30' }}>
                      <Trash2 size={14} style={{ color: 'var(--color-error)' }} />
                    </button>
                  </div>
                )}
              </button>
            );
          })}
        </div>
      </div>
    </div>
  );
}
