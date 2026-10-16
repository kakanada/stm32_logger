# Проверка длин описаний в logger_codes.h: каждое должно быть <= 64 байт UTF-8
# (LOGGER_MAX_DESCRIPTION_LENGTH), иначе LOGGER_Init() отклонит всю таблицу.
# Запуск: powershell -File check_descriptions.ps1 [путь\к\logger_codes.h]
# Код возврата 1 - есть нарушения (годится для CI/pre-commit).
param([string]$File = (Join-Path $PSScriptRoot 'logger_codes.h'))
$s = [IO.File]::ReadAllText($File, [Text.Encoding]::UTF8)
$bad = 0; $n = 0
foreach ($m in [regex]::Matches($s, '\{\s*(LOG_CODE_\w+),\s*LOGGER_PRIORITY_\w+,\s*"((?:[^"\\]|\\.)*)"')) {
    $n++
    $b = [Text.Encoding]::UTF8.GetByteCount($m.Groups[2].Value)
    if ($b -gt 64) { "$($m.Groups[1].Value): $b байт > 64: $($m.Groups[2].Value)"; $bad++ }
}
"Проверено описаний: $n, нарушений: $bad"
exit ([int]($bad -gt 0))
