interface TempGaugeProps {
  label: string;
  current: number;
  target: number;
  color: string;
  size?: number;
  maxTemp?: number;
}

export function TempGauge({ label, current, target, color, size = 140, maxTemp = 300 }: TempGaugeProps) {
  const radius = (size - 16) / 2;
  const cx = size / 2;
  const cy = size / 2;
  const circumference = 2 * Math.PI * radius;

  /* Arc goes from 135° to 405° (270° sweep) */
  const startAngle = 135;
  const sweepAngle = 270;

  const tempRatio = Math.min(current / maxTemp, 1);
  const targetRatio = target > 0 ? Math.min(target / maxTemp, 1) : 0;

  const trackDash = (sweepAngle / 360) * circumference;
  const valueDash = tempRatio * trackDash;
  const targetDash = targetRatio * trackDash;

  const isHeating = target > 0 && current < target - 2;
  const isAtTemp = target > 0 && Math.abs(current - target) <= 2;

  return (
    <div className="flex flex-col items-center gap-1">
      <svg width={size} height={size} viewBox={`0 0 ${size} ${size}`} className="drop-shadow-lg">
        {/* Subtle glow behind the arc */}
        <defs>
          <filter id={`glow-${label}`} x="-50%" y="-50%" width="200%" height="200%">
            <feGaussianBlur stdDeviation="4" result="blur" />
            <feMerge>
              <feMergeNode in="blur" />
              <feMergeNode in="SourceGraphic" />
            </feMerge>
          </filter>
        </defs>

        {/* Background track */}
        <circle
          cx={cx} cy={cy} r={radius}
          fill="none"
          stroke="var(--color-border)"
          strokeWidth={6}
          strokeDasharray={`${trackDash} ${circumference}`}
          strokeLinecap="round"
          transform={`rotate(${startAngle} ${cx} ${cy})`}
          opacity={0.5}
        />

        {/* Target marker */}
        {target > 0 && (
          <circle
            cx={cx} cy={cy} r={radius}
            fill="none"
            stroke={color}
            strokeWidth={6}
            strokeDasharray={`${targetDash} ${circumference}`}
            strokeLinecap="round"
            transform={`rotate(${startAngle} ${cx} ${cy})`}
            opacity={0.15}
          />
        )}

        {/* Current value arc */}
        <circle
          cx={cx} cy={cy} r={radius}
          fill="none"
          stroke={color}
          strokeWidth={6}
          strokeDasharray={`${valueDash} ${circumference}`}
          strokeLinecap="round"
          transform={`rotate(${startAngle} ${cx} ${cy})`}
          filter={current > 50 ? `url(#glow-${label})` : undefined}
          style={{ transition: 'stroke-dasharray 0.8s ease-out' }}
        />

        {/* Current temp — large mono readout */}
        <text
          x={cx} y={cy - 4}
          textAnchor="middle"
          dominantBaseline="central"
          fill="var(--color-text)"
          fontFamily="var(--font-mono)"
          fontWeight={700}
          fontSize={size * 0.2}
        >
          {current.toFixed(1)}°
        </text>

        {/* Target temp */}
        {target > 0 && (
          <text
            x={cx} y={cy + size * 0.15}
            textAnchor="middle"
            dominantBaseline="central"
            fill="var(--color-text-dim)"
            fontFamily="var(--font-mono)"
            fontSize={size * 0.1}
          >
            / {target}°
          </text>
        )}

        {/* Heating indicator dot */}
        {isHeating && (
          <circle
            cx={cx} cy={cy + size * 0.28}
            r={3}
            fill={color}
            style={{ animation: 'pulse-glow 1.2s ease-in-out infinite' }}
          />
        )}
        {isAtTemp && (
          <circle cx={cx} cy={cy + size * 0.28} r={3} fill="var(--color-success)" />
        )}
      </svg>

      <span
        className="text-xs font-medium tracking-wider uppercase"
        style={{ color: 'var(--color-text-dim)', fontFamily: 'var(--font-display)' }}
      >
        {label}
      </span>
    </div>
  );
}
