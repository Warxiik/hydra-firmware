import { useState, useEffect, useCallback, useRef } from 'react';

export interface PrinterState {
  state: 'idle' | 'homing' | 'heating' | 'printing' | 'paused' | 'error';
  nozzle0Temp: number;
  nozzle0Target: number;
  nozzle1Temp: number;
  nozzle1Target: number;
  bedTemp: number;
  bedTarget: number;
  progress: number;
  currentLayer: number;
  totalLayers: number;
  elapsedSeconds: number;
  remainingSeconds: number;
  fileName: string;
  speedOverride: number;
  flowOverride: number;
  fanSpeed: number;
  wifiConnected: boolean;
  wifiSSID: string;
  ipAddress: string;
  tempHistory: { time: number; n0: number; n1: number; bed: number }[];
}

const INITIAL_STATE: PrinterState = {
  state: 'idle',
  nozzle0Temp: 24.5,
  nozzle0Target: 0,
  nozzle1Temp: 23.8,
  nozzle1Target: 0,
  bedTemp: 25.1,
  bedTarget: 0,
  progress: 0,
  currentLayer: 0,
  totalLayers: 0,
  elapsedSeconds: 0,
  remainingSeconds: 0,
  fileName: '',
  speedOverride: 100,
  flowOverride: 100,
  fanSpeed: 0,
  wifiConnected: true,
  wifiSSID: 'Workshop-5G',
  ipAddress: '192.168.1.42',
  tempHistory: [],
};

/* Demo mode: simulate a printing state for UI development */
function useMockPrinter(): { state: PrinterState; updateState: (patch: Partial<PrinterState>) => void } {
  const [state, setState] = useState<PrinterState>({
    ...INITIAL_STATE,
    state: 'printing',
    nozzle0Temp: 208.3,
    nozzle0Target: 210,
    nozzle1Temp: 233.7,
    nozzle1Target: 235,
    bedTemp: 59.2,
    bedTarget: 60,
    progress: 0.42,
    currentLayer: 84,
    totalLayers: 200,
    elapsedSeconds: 2520,
    remainingSeconds: 3480,
    fileName: 'dual_towers_40+30mm',
    speedOverride: 100,
    flowOverride: 100,
    fanSpeed: 80,
  });

  useEffect(() => {
    const iv = setInterval(() => {
      setState(prev => {
        const now = Date.now() / 1000;
        const n0 = prev.nozzle0Target + (Math.sin(now * 0.5) * 1.5);
        const n1 = prev.nozzle1Target + (Math.cos(now * 0.3) * 1.8);
        const bed = prev.bedTarget + (Math.sin(now * 0.2) * 0.4);
        const newEntry = { time: now, n0, n1, bed };
        const history = [...prev.tempHistory, newEntry].slice(-120);
        return {
          ...prev,
          nozzle0Temp: Math.round(n0 * 10) / 10,
          nozzle1Temp: Math.round(n1 * 10) / 10,
          bedTemp: Math.round(bed * 10) / 10,
          elapsedSeconds: prev.elapsedSeconds + 1,
          remainingSeconds: Math.max(0, prev.remainingSeconds - 1),
          progress: Math.min(1, prev.progress + 0.00005),
          tempHistory: history,
        };
      });
    }, 1000);
    return () => clearInterval(iv);
  }, []);

  const updateState = useCallback((patch: Partial<PrinterState>) => {
    setState(prev => ({ ...prev, ...patch }));
  }, []);

  return { state, updateState };
}

export interface PrinterControls {
  startPrint: (fileName: string) => void;
  pausePrint: () => void;
  resumePrint: () => void;
  cancelPrint: () => void;
  homeAll: () => void;
  homeAxis: (axis: 'X' | 'Y' | 'Z') => void;
  setNozzleTemp: (nozzle: number, temp: number) => void;
  setBedTemp: (temp: number) => void;
  jog: (axis: string, distance: number) => void;
  setSpeedOverride: (percent: number) => void;
  setFlowOverride: (percent: number) => void;
  setFanSpeed: (percent: number) => void;
}

export function usePrinter(): { state: PrinterState; controls: PrinterControls; connected: boolean } {
  const wsRef = useRef<WebSocket | null>(null);
  const [connected, setConnected] = useState(false);
  const [liveState, setLiveState] = useState<PrinterState>(INITIAL_STATE);
  const historyRef = useRef<PrinterState['tempHistory']>([]);
  const mock = useMockPrinter();

  /* Connect to real WebSocket when available */
  useEffect(() => {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;

    try {
      const ws = new WebSocket(wsUrl);

      ws.onopen = () => {
        setConnected(true);
        wsRef.current = ws;
      };

      ws.onmessage = (e) => {
        try {
          const data = JSON.parse(e.data);
          /* Accumulate temp history from live updates */
          const now = Date.now() / 1000;
          historyRef.current = [
            ...historyRef.current,
            { time: now, n0: data.nozzle0Temp, n1: data.nozzle1Temp, bed: data.bedTemp },
          ].slice(-120);

          setLiveState({
            ...data,
            tempHistory: historyRef.current,
          });
        } catch { /* ignore malformed messages */ }
      };

      ws.onclose = () => {
        setConnected(false);
        wsRef.current = null;
      };

      ws.onerror = () => {
        setConnected(false);
      };

      return () => ws.close();
    } catch {
      /* WebSocket not available — mock mode */
    }
  }, []);

  const send = useCallback((type: string, payload: Record<string, unknown> = {}) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({ type, ...payload }));
    }
  }, []);

  const controls: PrinterControls = {
    startPrint: (fileName) => send('start_print', { fileName }),
    pausePrint: () => send('pause'),
    resumePrint: () => send('resume'),
    cancelPrint: () => send('cancel'),
    homeAll: () => send('home', { axes: 'XYZ' }),
    homeAxis: (axis) => send('home', { axes: axis }),
    setNozzleTemp: (nozzle, temp) => send('set_temp', { nozzle, temp }),
    setBedTemp: (temp) => send('set_temp', { target: 'bed', temp }),
    jog: (axis, distance) => send('jog', { axis, distance }),
    setSpeedOverride: (percent) => send('set_speed', { percent }),
    setFlowOverride: (percent) => send('set_flow', { percent }),
    setFanSpeed: (percent) => send('set_fan', { percent }),
  };

  /* Use live data when connected, mock data when not */
  return { state: connected ? liveState : mock.state, controls, connected };
}
