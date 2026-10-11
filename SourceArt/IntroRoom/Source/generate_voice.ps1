param([string]$RawOutputFolder = "$env:TEMP\GhostOfYourselfIntroVoice")
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Speech
New-Item -ItemType Directory -Force -Path $RawOutputFolder | Out-Null
$lines = @(
    'Hello, Subject zero three. Welcome to the Echo Research Facility.',
    'I am your assessment assistant. I am delighted you volunteered. Your file says you did.',
    'Our system can record your actions and replay them as an echo. Think of it as help from your past self.',
    'You will need that help. Some doors require you to be in two places at once.',
    'Your first assessment is through this door. Take your time. We have already waited quite a while.',
    'And if any of this feels familiar, please continue as normal. We will handle the remembering.'
)
$voice = New-Object System.Speech.Synthesis.SpeechSynthesizer
try {
    $voice.SelectVoice('Microsoft Zira Desktop')
    $voice.Rate = -1
    $voice.Volume = 100
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $voice.SetOutputToWaveFile((Join-Path $RawOutputFolder ("Line{0}.wav" -f $i)))
        $voice.Speak($lines[$i])
        $voice.SetOutputToNull()
    }
    $lines | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $RawOutputFolder 'Lines.json')
} finally { $voice.Dispose() }
Write-Host "Generated voice segments in $RawOutputFolder"
