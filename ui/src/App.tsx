import { useState } from 'react';
import { Home, Wrench, Printer, FolderOpen, Settings, Move } from 'lucide-react';
import { StatusBar } from './components/StatusBar';
import { HomePage } from './pages/HomePage';
import { PrintPage } from './pages/PrintPage';
import { PreparePage } from './pages/PreparePage';
import { MovePage } from './pages/MovePage';
import { FilesPage } from './pages/FilesPage';
import { SettingsPage } from './pages/SettingsPage';
import { usePrinter } from './hooks/usePrinter';

type Page = 'home' | 'prepare' | 'print' | 'files' | 'settings' | 'move';

const NAV_ITEMS: { id: Page; label: string; icon: typeof Home }[] = [
  { id: 'home',    label: 'Home',    icon: Home },
  { id: 'prepare', label: 'Prepare', icon: Wrench },
  { id: 'print',   label: 'Print',   icon: Printer },
  { id: 'move',    label: 'Move',    icon: Move },
  { id: 'files',   label: 'Files',   icon: FolderOpen },
  { id: 'settings', label: 'Settings', icon: Settings },
];

export default function App() {
  const [page, setPage] = useState<Page>('home');
  const { state, controls, connected } = usePrinter();

  const renderPage = () => {
    switch (page) {
      case 'home':     return <HomePage state={state} controls={controls} />;
      case 'prepare':  return <PreparePage state={state} controls={controls} />;
      case 'print':    return <PrintPage state={state} controls={controls} />;
      case 'move':     return <MovePage controls={controls} />;
      case 'files':    return <FilesPage controls={controls} />;
      case 'settings': return <SettingsPage state={state} />;
    }
  };

  return (
    <>
      <StatusBar state={state} connected={connected} />

      <main className="flex-1 overflow-hidden">
        {renderPage()}
      </main>

      {/* Bottom navigation */}
      <nav className="flex items-stretch shrink-0"
        style={{
          height: 56,
          background: 'var(--color-surface)',
          borderTop: '1px solid var(--color-border-subtle)',
        }}>
        {NAV_ITEMS.map(item => {
          const active = page === item.id;
          const Icon = item.icon;
          return (
            <button
              key={item.id}
              onClick={() => setPage(item.id)}
              className="flex-1 flex flex-col items-center justify-center gap-1 transition-all cursor-pointer relative"
              style={{ background: 'transparent', border: 'none' }}
            >
              {active && (
                <div className="absolute top-0 left-1/2 -translate-x-1/2 w-8 h-[2px] rounded-b-full"
                  style={{
                    background: 'var(--color-accent)',
                    boxShadow: '0 2px 8px var(--color-accent-glow-strong)',
                  }} />
              )}

              <Icon
                size={18}
                style={{
                  color: active ? 'var(--color-accent)' : 'var(--color-text-muted)',
                  transition: 'color 0.2s',
                }}
              />
              <span
                className="text-[10px] font-medium tracking-wide"
                style={{
                  color: active ? 'var(--color-accent)' : 'var(--color-text-muted)',
                  fontFamily: 'var(--font-display)',
                  transition: 'color 0.2s',
                }}
              >
                {item.label}
              </span>
            </button>
          );
        })}
      </nav>
    </>
  );
}
