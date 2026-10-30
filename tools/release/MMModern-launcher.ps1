# MMModern launcher. Keep ASCII for Windows PowerShell 5.1; WPF needs -STA.
# Dot-source to use the non-UI functions without opening a window.
$ErrorActionPreference = 'Stop'
function ConvertFrom-MMReport([string]$Text) {
    $result = @{}
    foreach ($line in ($Text -split '\r?\n')) {
        if ($line -match '^([^:]+): (.*)$') { $result[$Matches[1]] = $Matches[2] }
    }
    return $result
}
function ConvertTo-MMArgument([AllowEmptyString()][string]$Value) {
    # CRT quoting, including trailing backslashes and embedded quotes.
    return '"' + ([regex]::Replace(([regex]::Replace($Value, '(\\*)"', '$1$1\"')), '(\\+)$', '$1$1')) + '"'
}
function Get-MMGameArguments([string]$Folder) { return ConvertTo-MMArgument $Folder }
# The installer's 0/1 answer to the volume question, shown as No/Yes.
function ConvertTo-MMVolumeLabel([string]$Value) { if ($Value -eq '1') { 'Yes' } else { 'No' } }
function ConvertFrom-MMVolumeLabel([string]$Label) { if ($Label -eq 'Yes') { '1' } else { '0' } }
function Test-MMRomsFound([hashtable]$Facts) {
    return ($Facts['roms.control'] -like 'identified*') -and ($Facts['roms.pcm'] -like 'identified*')
}
# One installer menu as the window lists it: devices MMModern plays in the
# installer's order, then None, then the devices it keeps silent. Report
# captions end in "; played" or "; silent"; the menu keys stay hidden.
function Get-MMDeviceChoices([hashtable]$Rules, [string]$Kind) {
    $recommended = if ($Kind -eq 'music') { '3' } else { '10' }
    $available = @(); $none = @(); $silent = @()
    foreach ($key in 1,2,3,4,5,6,7,8,9,0) {
        $name, $status = $Rules["$Kind.$key.caption"] -split '; ', 2
        $id = $Rules["$Kind.$key.id"]
        $label = $name
        if ($id -eq $recommended) { $label = "$name (recommended)" }
        elseif ($Kind -eq 'music' -and $id -eq '4') { $label = "$name (needs MT-32 ROMs)" }
        $choice = [pscustomobject]@{ Key="$key"; Id=$id; Name=$name; Label=$label; Silent=($status -eq 'silent' -and $name -ne 'None'); Recommended=($id -eq $recommended) }
        if ($name -eq 'None') { $none += $choice } elseif ($choice.Silent) { $silent += $choice } else { $available += $choice }
    }
    return @($available + $none + $silent)
}
# The explanation under a device list for the chosen row.
function Get-MMDeviceHint($Choice, [string]$Kind, [bool]$RomsFound) {
    if ($Choice.Silent) { return 'MMModern does not play this device yet. The game stays silent and shows a notice.' }
    if ($Choice.Name -eq 'None') { if ($Kind -eq 'music') { return 'No music or sound effects.' } else { return 'No speech or digital sounds.' } }
    if ($Kind -eq 'music' -and $Choice.Id -eq '4') {
        if ($RomsFound) { return 'Your MT-32 ROMs were found.' }
        return 'Put your MT-32 1.07 control ROM and MT-32 PCM ROM in the ROM folder. Without them the music is silent.'
    }
    if ($Choice.Recommended) { return 'The device the CD edition installs.' }
    return ''
}
# The main window's summary: one row per fact, each 'ok', 'warn' or 'info'.
function Get-MMStatusRows([hashtable]$Facts) {
    $rows = @()
    $row = { param($Level,$Title,$Value,$Detail) [pscustomobject]@{ Level=$Level; Title=$Title; Value=$Value; Detail=$Detail } }
    if ($Facts['game.status'] -eq 'usable') {
        if ($Facts['game.exe'] -and $Facts['game.exe'] -notlike 'usable*') {
            $rows += & $row 'warn' 'Game' $Facts['game.edition'] "$($Facts['game.folder'])`nIntros and ending unavailable: XEEN.EXE could not be read."
        } else { $rows += & $row 'ok' 'Game' $Facts['game.edition'] $Facts['game.folder'] }
    } else { $rows += & $row 'warn' 'Game' 'No usable game data' ($Facts['game.status'] -replace '^unusable: ', '') }
    $roms = Test-MMRomsFound $Facts
    if ($Facts['cfg.error']) {
        $rows += & $row 'warn' 'Sound' 'Sound setup file damaged' 'Open Sound setup and choose Reset to installed.'
    } elseif ($Facts['sound.error']) {
        $rows += & $row 'warn' 'Sound' 'Sound settings unreadable' $Facts['sound.error']
    } else {
        $source = if ($Facts['cfg.source'] -eq 'owned') { 'Chosen in Sound setup' } else { 'From the game installation' }
        foreach ($pair in @(@('Music','sound.music'), @('Speech','sound.speech'))) {
            $name, $status = $Facts[$pair[1]] -split '; ', 2
            if ($status -eq 'silent' -and $name -ne 'None') { $rows += & $row 'warn' $pair[0] $name 'No sound in MMModern yet' }
            elseif ($name -like '*MT-32*' -and -not $roms) { $rows += & $row 'warn' $pair[0] $name 'Needs your MT-32 ROMs' }
            else { $rows += & $row 'ok' $pair[0] $name $source }
        }
    }
    if ($roms) { $rows += & $row 'ok' 'MT-32 ROMs' 'Found' $Facts['roms.folder'] }
    elseif ($Facts['roms.control'] -eq 'missing' -and $Facts['roms.pcm'] -eq 'missing') {
        $rows += & $row 'info' 'MT-32 ROMs' 'Not found' "Only needed for MT-32 music.`n$($Facts['roms.folder'])"
    } else { $rows += & $row 'warn' 'MT-32 ROMs' 'Not recognized' "Needs the MT-32 1.07 control ROM and the MT-32 PCM ROM.`n$($Facts['roms.folder'])" }
    $used = 0; [void][int]::TryParse([string]$Facts['saves.used-slots'], [ref]$used)
    $saves = if ($used -eq 0) { 'No saves yet' } else { "$used of 10 slots used" }
    $rows += & $row 'info' 'Saves' $saves $Facts['saves.folder']
    return $rows
}
function Get-MMGameWindowHandle($Process) {
    if ($Process.HasExited) { return [IntPtr]::Zero }
    try {
        $handle = $Process.MainWindowHandle
        if ($null -ne $handle) { return [IntPtr]$handle }
    } catch { }
    # A process can exit between HasExited and the window query.
    return [IntPtr]::Zero
}
function Invoke-MMCommand([string]$Exe, [string[]]$CommandArguments) {
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = $Exe
    $info.Arguments = (@($CommandArguments | ForEach-Object { ConvertTo-MMArgument $_ }) -join ' ')
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.StandardOutputEncoding = New-Object Text.UTF8Encoding($false)
    $info.StandardErrorEncoding = New-Object Text.UTF8Encoding($false)
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $info
    try {
        [void]$process.Start()
        $output = $process.StandardOutput.ReadToEndAsync()
        $errorText = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) { throw $errorText.Result.Trim() }
        return $output.Result
    } finally { $process.Dispose() }
}
function Test-GameFolder([string]$Folder) {
    $app=$script:MMState
    if (-not $Folder) { return $false }
    try { return (ConvertFrom-MMReport (Invoke-MMCommand $app.Exe @('--diagnostics',$Folder)))['game.status'] -eq 'usable' }
    catch { return $false }
}
function Resolve-GameFolder([string]$Folder) {
    if (Test-GameFolder $Folder) { return $Folder }
    if ($Folder -and (Split-Path -Leaf $Folder) -eq 'WORLD') {
        $parent = Split-Path -Path $Folder -Parent
        if (Test-GameFolder $parent) { return $parent }
    }
    return $null
}
function Find-GogFolder {
    foreach ($root in 'HKLM:\SOFTWARE\WOW6432Node\GOG.com\Games','HKLM:\SOFTWARE\GOG.com\Games','HKCU:\SOFTWARE\GOG.com\Games') {
        if (-not (Test-Path $root)) { continue }
        foreach ($key in Get-ChildItem $root -ErrorAction SilentlyContinue) {
            $entry = Get-ItemProperty -LiteralPath $key.PSPath -ErrorAction SilentlyContinue
            if ($entry -and $entry.path -and (Test-GameFolder $entry.path)) { return $entry.path }
        }
    }
    return $null
}
function Show-MMMessage([string]$Text, [string]$Title = 'MMModern', [string]$Buttons = 'OK', [string]$Icon = 'Information') {
    $owner = $script:MMState.Window
    if ($owner -and $owner.IsVisible) { return [Windows.MessageBox]::Show($owner, $Text, $Title, $Buttons, $Icon) }
    return [Windows.MessageBox]::Show($Text, $Title, $Buttons, $Icon)
}
function Select-GameFolder {
    $dialog = New-Object Windows.Forms.FolderBrowserDialog
    $dialog.Description = 'Choose the folder containing your World of Xeen CD game data.'
    $dialog.ShowNewFolderButton = $false
    try {
        while ($dialog.ShowDialog() -eq 'OK') {
            $folder = Resolve-GameFolder $dialog.SelectedPath
            if ($folder) { return $folder }
            [void](Show-MMMessage 'No usable World of Xeen CD data found. Choose the game installation folder.' 'MMModern' 'OK' 'Warning')
        }
        return $null
    } finally { $dialog.Dispose() }
}
function Open-MMFolder([string]$Path) {
    Start-Process explorer.exe -ArgumentList (ConvertTo-MMArgument $Path) -WindowStyle Normal
}

# Windows 11-like light styles shared by both windows.
$script:MMStyles = @'
<ResourceDictionary xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
  <Style TargetType="Button">
    <Setter Property="Background" Value="#FFFFFF"/>
    <Setter Property="BorderBrush" Value="#D1D1D1"/>
    <Setter Property="BorderThickness" Value="1"/>
    <Setter Property="Foreground" Value="#1A1A1A"/>
    <Setter Property="Padding" Value="16,8"/>
    <Setter Property="Cursor" Value="Hand"/>
    <Setter Property="FocusVisualStyle" Value="{x:Null}"/>
    <Setter Property="Template">
      <Setter.Value>
        <ControlTemplate TargetType="Button">
          <Border Background="{TemplateBinding Background}" BorderBrush="{TemplateBinding BorderBrush}"
                  BorderThickness="{TemplateBinding BorderThickness}" CornerRadius="6" Padding="{TemplateBinding Padding}">
            <ContentPresenter HorizontalAlignment="Center" VerticalAlignment="Center"/>
          </Border>
        </ControlTemplate>
      </Setter.Value>
    </Setter>
    <Style.Triggers>
      <Trigger Property="IsMouseOver" Value="True"><Setter Property="Background" Value="#F5F5F5"/></Trigger>
      <Trigger Property="IsPressed" Value="True"><Setter Property="Background" Value="#EBEBEB"/></Trigger>
      <Trigger Property="IsKeyboardFocused" Value="True"><Setter Property="BorderBrush" Value="#005FB8"/><Setter Property="BorderThickness" Value="2"/></Trigger>
      <Trigger Property="IsEnabled" Value="False"><Setter Property="Opacity" Value="0.5"/></Trigger>
    </Style.Triggers>
  </Style>
  <Style x:Key="Accent" TargetType="Button" BasedOn="{StaticResource {x:Type Button}}">
    <Setter Property="Background" Value="#005FB8"/>
    <Setter Property="BorderBrush" Value="#005FB8"/>
    <Setter Property="Foreground" Value="#FFFFFF"/>
    <Setter Property="FontWeight" Value="SemiBold"/>
    <Style.Triggers>
      <Trigger Property="IsMouseOver" Value="True"><Setter Property="Background" Value="#1A6FC0"/></Trigger>
      <Trigger Property="IsPressed" Value="True"><Setter Property="Background" Value="#004C94"/></Trigger>
      <Trigger Property="IsKeyboardFocused" Value="True"><Setter Property="BorderBrush" Value="#003A70"/><Setter Property="BorderThickness" Value="2"/></Trigger>
    </Style.Triggers>
  </Style>
  <Style x:Key="Link" TargetType="Button">
    <Setter Property="Foreground" Value="#005FB8"/>
    <Setter Property="Cursor" Value="Hand"/>
    <Setter Property="Padding" Value="6,2"/>
    <Setter Property="Template">
      <Setter.Value>
        <ControlTemplate TargetType="Button">
          <Border Background="Transparent" Padding="{TemplateBinding Padding}"><ContentPresenter/></Border>
        </ControlTemplate>
      </Setter.Value>
    </Setter>
    <Style.Triggers>
      <Trigger Property="IsMouseOver" Value="True"><Setter Property="Foreground" Value="#003A70"/></Trigger>
      <Trigger Property="IsEnabled" Value="False"><Setter Property="Opacity" Value="0.5"/></Trigger>
    </Style.Triggers>
  </Style>
  <Style x:Key="Card" TargetType="Border">
    <Setter Property="Background" Value="#FFFFFF"/>
    <Setter Property="BorderBrush" Value="#E5E5E5"/>
    <Setter Property="BorderThickness" Value="1"/>
    <Setter Property="CornerRadius" Value="8"/>
  </Style>
  <Style x:Key="Note" TargetType="TextBlock">
    <Setter Property="Foreground" Value="#5F5F5F"/>
    <Setter Property="FontSize" Value="13"/>
    <Setter Property="TextWrapping" Value="Wrap"/>
  </Style>
</ResourceDictionary>
'@
$script:MMMainXaml = @'
<Window xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Title="MMModern" Width="620" SizeToContent="Height" ResizeMode="CanMinimize"
        WindowStartupLocation="CenterScreen" Background="#F3F3F3" FontFamily="Segoe UI" FontSize="14"
        UseLayoutRounding="True" SnapsToDevicePixels="True">
  <!--STYLES-->
  <StackPanel Margin="28,24,28,20">
    <StackPanel Orientation="Horizontal">
      <TextBlock Text="MMModern" FontSize="28" FontWeight="SemiBold" Foreground="#1A1A1A"/>
      <TextBlock x:Name="Version" FontSize="15" Foreground="#5F5F5F" VerticalAlignment="Bottom" Margin="10,0,0,6"/>
    </StackPanel>
    <Border Style="{StaticResource Card}" Margin="0,18,0,0" Padding="18,6">
      <StackPanel x:Name="Status"/>
    </Border>
    <Button x:Name="Play" Style="{StaticResource Accent}" Content="Play" IsDefault="True" FontSize="17" Height="50" Margin="0,20,0,0"/>
    <Grid Margin="0,10,0,0">
      <Grid.ColumnDefinitions><ColumnDefinition/><ColumnDefinition Width="10"/><ColumnDefinition/></Grid.ColumnDefinitions>
      <Button x:Name="SoundSetup" Grid.Column="0" Content="Sound setup"/>
      <Button x:Name="ChangeFolder" Grid.Column="2" Content="Change game folder"/>
    </Grid>
    <StackPanel Orientation="Horizontal" HorizontalAlignment="Center" Margin="0,14,0,0">
      <Button x:Name="CopyDiagnostics" Style="{StaticResource Link}" Content="Copy diagnostics"/>
      <TextBlock Text="&#x00B7;" Foreground="#8A8A8A" VerticalAlignment="Center"/>
      <Button x:Name="OpenSaves" Style="{StaticResource Link}" Content="Open saves folder"/>
      <TextBlock Text="&#x00B7;" Foreground="#8A8A8A" VerticalAlignment="Center"/>
      <Button x:Name="OpenLog" Style="{StaticResource Link}" Content="Open log folder"/>
    </StackPanel>
    <TextBlock x:Name="Notice" Style="{StaticResource Note}" HorizontalAlignment="Center" Margin="0,6,0,0"/>
  </StackPanel>
</Window>
'@
$script:MMSoundXaml = @'
<Window xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Title="Sound setup" Width="560" SizeToContent="Height" ResizeMode="NoResize" ShowInTaskbar="False"
        WindowStartupLocation="CenterOwner" Background="#F3F3F3" FontFamily="Segoe UI" FontSize="14"
        UseLayoutRounding="True" SnapsToDevicePixels="True">
  <!--STYLES-->
  <StackPanel Margin="28,24,28,20">
    <TextBlock Text="Sound setup" FontSize="22" FontWeight="SemiBold" Foreground="#1A1A1A"/>
    <TextBlock Style="{StaticResource Note}" FontSize="14" Margin="0,4,0,0"
               Text="Choose from the devices the original installer offered. The choice is saved for MMModern only; your game folder is not changed."/>
    <Border Style="{StaticResource Card}" Margin="0,16,0,0" Padding="18,16">
      <StackPanel>
        <TextBlock Text="Music and sound effects" FontWeight="SemiBold"/>
        <ComboBox x:Name="Music" Margin="0,6,0,0" Padding="8,6"/>
        <TextBlock x:Name="MusicHint" Style="{StaticResource Note}" Margin="0,6,0,0"/>
        <Button x:Name="OpenRoms" Style="{StaticResource Link}" Content="Open the ROM folder" HorizontalAlignment="Left" Margin="-6,2,0,0"/>
        <TextBlock Text="Speech and digital sounds" FontWeight="SemiBold" Margin="0,16,0,0"/>
        <ComboBox x:Name="Speech" Margin="0,6,0,0" Padding="8,6"/>
        <TextBlock x:Name="SpeechHint" Style="{StaticResource Note}" Margin="0,6,0,0"/>
      </StackPanel>
    </Border>
    <Expander x:Name="Advanced" Header="Advanced: original hardware settings" Margin="0,14,0,0">
      <StackPanel Margin="0,8,0,0">
        <TextBlock Style="{StaticResource Note}"
                   Text="These told the game how the original DOS sound cards were set up. They do not change the sound quality in MMModern; leave them as they are."/>
        <Grid x:Name="Questions" Margin="0,8,0,0">
          <Grid.ColumnDefinitions><ColumnDefinition/><ColumnDefinition Width="150"/></Grid.ColumnDefinitions>
        </Grid>
      </StackPanel>
    </Expander>
    <DockPanel Margin="0,20,0,0" LastChildFill="False">
      <Button x:Name="Reset" DockPanel.Dock="Left" Content="Reset to installed"/>
      <Button x:Name="Save" DockPanel.Dock="Right" Style="{StaticResource Accent}" Content="Save" IsDefault="True" MinWidth="110" Margin="8,0,0,0"/>
      <Button x:Name="Cancel" DockPanel.Dock="Right" Content="Cancel" IsCancel="True" MinWidth="110"/>
    </DockPanel>
  </StackPanel>
</Window>
'@
# Styles go in before parsing, so StaticResource references resolve.
function New-MMWindow([string]$Xaml) {
    return [Windows.Markup.XamlReader]::Parse($Xaml.Replace('<!--STYLES-->', "<Window.Resources>$($script:MMStyles)</Window.Resources>"))
}
# Fills the main window from the latest diagnostics report.
function Update-MMView {
    $app=$script:MMState
    $w=$app.Window
    $w.Title = $app.Facts['version']
    $w.FindName('Version').Text = ($app.Facts['version'] -replace '^MMModern ', '')
    $panel = $w.FindName('Status')
    $panel.Children.Clear()
    $glyphs = @{ ok=[string][char]0xE73E; warn=[string][char]0xE7BA; info=[string][char]0xE946 }
    $colors = @{ ok='#0F7B0F'; warn='#9D5D00'; info='#5F5F5F' }
    $first = $true
    foreach ($row in Get-MMStatusRows $app.Facts) {
        $grid = New-Object Windows.Controls.Grid
        $grid.Margin = '0,10,0,10'
        foreach ($width in 30,110,0) {
            $column = New-Object Windows.Controls.ColumnDefinition
            if ($width) { $column.Width = $width } else { $column.Width = '*' }
            $grid.ColumnDefinitions.Add($column)
        }
        $icon = New-Object Windows.Controls.TextBlock -Property @{ Text=$glyphs[$row.Level]; FontFamily='Segoe MDL2 Assets'; FontSize=16; Foreground=$colors[$row.Level]; Margin='0,2,0,0' }
        $title = New-Object Windows.Controls.TextBlock -Property @{ Text=$row.Title; Foreground='#5F5F5F' }
        $values = New-Object Windows.Controls.StackPanel
        [void]$values.Children.Add((New-Object Windows.Controls.TextBlock -Property @{ Text=$row.Value; FontWeight='SemiBold'; Foreground='#1A1A1A'; TextWrapping='Wrap' }))
        if ($row.Detail) { [void]$values.Children.Add((New-Object Windows.Controls.TextBlock -Property @{ Text=$row.Detail; Foreground='#5F5F5F'; FontSize=12.5; TextWrapping='Wrap'; Margin='0,2,0,0' })) }
        [Windows.Controls.Grid]::SetColumn($title, 1); [Windows.Controls.Grid]::SetColumn($values, 2)
        [void]$grid.Children.Add($icon); [void]$grid.Children.Add($title); [void]$grid.Children.Add($values)
        if (-not $first) { [void]$panel.Children.Add((New-Object Windows.Controls.Border -Property @{ Height=1; Background='#EDEDED' })) }
        [void]$panel.Children.Add($grid)
        $first = $false
    }
}
function Update-MMReport {
    $app=$script:MMState
    $app.Report = Invoke-MMCommand $app.Exe @('--diagnostics',$app.Folder)
    $app.Facts = ConvertFrom-MMReport $app.Report
    Update-MMView
}
function New-MMSoundWindow([hashtable]$Rules, [hashtable]$Facts) {
    $dialog = New-MMWindow $script:MMSoundXaml
    $sound = @{ Window=$dialog; Rules=$Rules; RomsFound=(Test-MMRomsFound $Facts); RomFolder=$Facts['roms.folder']; Fields=@{}; Lists=@{} }
    $script:MMSound = $sound
    foreach ($kind in 'music','speech') {
        $list = $dialog.FindName($(if ($kind -eq 'music') { 'Music' } else { 'Speech' }))
        $separated = $false
        foreach ($choice in Get-MMDeviceChoices $Rules $kind) {
            if ($choice.Silent -and -not $separated) {
                $header = New-Object Windows.Controls.ComboBoxItem -Property @{ Content='No sound in MMModern yet'; IsEnabled=$false; FontStyle='Italic'; FontSize=12.5 }
                [void]$list.Items.Add($header); $separated = $true
            }
            $item = New-Object Windows.Controls.ComboBoxItem -Property @{ Content=$choice.Label; Tag=$choice }
            if ($choice.Silent) { $item.Padding = '18,4,4,4' }
            [void]$list.Items.Add($item)
            if ($choice.Key -eq $Rules["current.$kind"]) { $list.SelectedItem = $item }
        }
        if ($list.SelectedIndex -lt 0) { $list.SelectedIndex = 0 }
        $sound.Lists[$kind] = $list
    }
    foreach ($list in $sound.Lists.Values) { $list.Add_SelectionChanged({ Update-MMSoundQuestions }) }
    Update-MMSoundQuestions
    $dialog.FindName('OpenRoms').Add_Click({
        $path = [Environment]::ExpandEnvironmentVariables($script:MMSound.RomFolder)
        New-Item -ItemType Directory -Force $path | Out-Null
        Open-MMFolder $path
    })
    return $dialog
}
# Rebuilds the hints and the installer's questions for the chosen rows.
function Update-MMSoundQuestions {
    $sound = $script:MMSound
    $dialog = $sound.Window
    $grid = $dialog.FindName('Questions')
    $grid.Children.Clear(); $grid.RowDefinitions.Clear(); $sound.Fields.Clear()
    $row = 0
    foreach ($kind in 'music','speech') {
        $choice = $sound.Lists[$kind].SelectedItem.Tag
        $hint = $dialog.FindName($(if ($kind -eq 'music') { 'MusicHint' } else { 'SpeechHint' }))
        $hint.Text = Get-MMDeviceHint $choice $kind $sound.RomsFound
        $hint.Visibility = if ($hint.Text) { 'Visible' } else { 'Collapsed' }
        $hint.Foreground = if ($choice.Silent -or ($kind -eq 'music' -and $choice.Id -eq '4' -and -not $sound.RomsFound)) { '#9D5D00' } else { '#5F5F5F' }
        if ($kind -eq 'music') { $dialog.FindName('OpenRoms').Visibility = if ($choice.Id -eq '4') { 'Visible' } else { 'Collapsed' } }
        foreach ($field in 'digital-port','irq','dma','volume','music-port') {
            $values = $sound.Rules["$kind.$($choice.Key).$field"]
            if (-not $values) { continue }
            $grid.RowDefinitions.Add((New-Object Windows.Controls.RowDefinition))
            $caption = switch ($field) { 'digital-port' {'Speech card port'}; 'irq' {'IRQ'}; 'dma' {'DMA'}; 'volume' {'Automatically adjust Sound Blaster volumes?'}; 'music-port' {'Music card port'} }
            $label = New-Object Windows.Controls.TextBlock -Property @{ Text=$caption; VerticalAlignment='Center'; TextWrapping='Wrap'; Margin='0,4,12,4' }
            $list = New-Object Windows.Controls.ComboBox -Property @{ Margin='0,4,0,4'; Padding='8,4' }
            if ($field -eq 'volume') {
                foreach ($value in 'Yes','No') { [void]$list.Items.Add($value) }
                $list.SelectedItem = ConvertTo-MMVolumeLabel $sound.Rules['current.volume']
            } else {
                foreach ($value in ($values -split ',')) { [void]$list.Items.Add($value) }
                $list.SelectedItem = $sound.Rules["current.$field"]
            }
            if ($list.SelectedIndex -lt 0) { $list.SelectedIndex = 0 }
            [Windows.Controls.Grid]::SetRow($label, $row); [Windows.Controls.Grid]::SetRow($list, $row); [Windows.Controls.Grid]::SetColumn($list, 1)
            [void]$grid.Children.Add($label); [void]$grid.Children.Add($list)
            $sound.Fields[$field] = $list
            $row++
        }
    }
    $dialog.FindName('Advanced').Visibility = if ($row) { 'Visible' } else { 'Collapsed' }
}
function Show-MMSoundSetup {
    $app=$script:MMState
    try { $rules = ConvertFrom-MMReport (Invoke-MMCommand $app.Exe @('--sound-setup',$app.Folder)) }
    catch {
        $choice = Show-MMMessage "$($_.Exception.Message)`n`nReset the sound setup to the installed devices?" 'Sound setup' 'YesNo' 'Warning'
        if ($choice -eq 'Yes') {
            try { [void](Invoke-MMCommand $app.Exe @('--sound-setup',$app.Folder,'--reset')) }
            catch { [void](Show-MMMessage $_.Exception.Message 'Sound setup' 'OK' 'Error') }
            Update-MMReport
        }
        return
    }
    $dialog = New-MMSoundWindow $rules $app.Facts
    $dialog.Owner = $app.Window
    $dialog.FindName('Save').Add_Click({
        $sound = $script:MMSound
        try {
            $arguments = @('--sound-setup',$script:MMState.Folder)
            foreach ($kind in 'speech','music') { $arguments += @("--$kind",$sound.Lists[$kind].SelectedItem.Tag.Key) }
            foreach ($field in 'digital-port','irq','dma','volume','music-port') {
                if (-not $sound.Fields.ContainsKey($field)) { continue }
                $answer = [string]$sound.Fields[$field].SelectedItem
                if ($field -eq 'volume') { $answer = ConvertFrom-MMVolumeLabel $answer }
                $arguments += @("--$field",$answer)
            }
            [void](Invoke-MMCommand $script:MMState.Exe $arguments)
            $sound.Window.DialogResult = $true
        } catch { [void][Windows.MessageBox]::Show($sound.Window,$_.Exception.Message,'Sound setup','OK','Error') }
    })
    $dialog.FindName('Reset').Add_Click({
        $sound = $script:MMSound
        try {
            [void](Invoke-MMCommand $script:MMState.Exe @('--sound-setup',$script:MMState.Folder,'--reset'))
            $sound.Window.DialogResult = $true
        } catch { [void][Windows.MessageBox]::Show($sound.Window,$_.Exception.Message,'Sound setup','OK','Error') }
    })
    [void]$dialog.ShowDialog()
    Update-MMReport
}
function Set-MMRunning([bool]$Running) {
    $w=$script:MMState.Window
    foreach ($name in 'Play','SoundSetup','ChangeFolder','CopyDiagnostics','OpenSaves','OpenLog') { $w.FindName($name).IsEnabled = -not $Running }
    $w.FindName('Play').Content = if ($Running) { 'Playing...' } else { 'Play' }
}
function Start-MMLauncher {
    $script:MMState=@{}
    $app=$script:MMState
    Add-Type -AssemblyName PresentationFramework,PresentationCore,WindowsBase,System.Windows.Forms
    Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public static class MMForeground { [DllImport("user32.dll")] public static extern bool AllowSetForegroundWindow(int pid); [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr window); }'
    [Windows.Forms.Application]::EnableVisualStyles()
    $app.Exe = Join-Path $PSScriptRoot 'mmodern.exe'
    $settings = Join-Path $env:LOCALAPPDATA 'MMModern'
    if ($env:MMODERN_PLAYER_DIR) { $settings=$env:MMODERN_PLAYER_DIR }
    $app.Remembered = Join-Path $settings 'game-folder.txt'
    $app.Settings = $settings
    $app.Folder=$null
    if (Test-Path -LiteralPath $app.Remembered -PathType Leaf) {
        $saved=Get-Content -LiteralPath $app.Remembered -Encoding UTF8 -TotalCount 1
        if ($saved -and (Test-GameFolder $saved.Trim())) { $app.Folder=$saved.Trim() }
    }
    if (-not $app.Folder) { $app.Folder=Find-GogFolder }
    if (-not $app.Folder) { $app.Folder=Select-GameFolder }
    if (-not $app.Folder) { return }
    $remember = { New-Item -ItemType Directory -Force $script:MMState.Settings | Out-Null; Set-Content -LiteralPath $script:MMState.Remembered -Value $script:MMState.Folder -Encoding UTF8 }
    & $remember
    $app.Remember = $remember
    $w = New-MMWindow $script:MMMainXaml
    $app.Window = $w
    Update-MMReport
    $w.FindName('SoundSetup').Add_Click({ Show-MMSoundSetup })
    $w.FindName('ChangeFolder').Add_Click({
        $chosen=Select-GameFolder
        if ($chosen) { $script:MMState.Folder=$chosen; & $script:MMState.Remember; Update-MMReport }
    })
    $w.FindName('CopyDiagnostics').Add_Click({
        $app=$script:MMState
        try { Update-MMReport; [Windows.Clipboard]::SetText($app.Report); $app.Window.FindName('Notice').Text='Diagnostics copied. Paste them into your bug report.' }
        catch { $app.Window.FindName('Notice').Text="Could not copy the diagnostics: $($_.Exception.Message)" }
    })
    $w.FindName('OpenSaves').Add_Click({
        $path=[Environment]::ExpandEnvironmentVariables($script:MMState.Facts['saves.folder'])
        if ($path -and (Test-Path -LiteralPath $path -PathType Container)) { Open-MMFolder $path }
        else { $script:MMState.Window.FindName('Notice').Text='The saves folder is created when you first save a game.' }
    })
    $w.FindName('OpenLog').Add_Click({
        $path=Split-Path -Path ([Environment]::ExpandEnvironmentVariables($script:MMState.Facts['log.file'])) -Parent
        if (Test-Path -LiteralPath $path -PathType Container) { Open-MMFolder $path }
        else { $script:MMState.Window.FindName('Notice').Text='The log folder is created when the game starts.' }
    })
    $app.Process=$null; $app.Raised=$false
    $app.Timer=New-Object Windows.Threading.DispatcherTimer
    $app.Timer.Interval=[TimeSpan]::FromMilliseconds(200)
    $w.FindName('Play').Add_Click({
        $app=$script:MMState
        try {
            $info=New-Object Diagnostics.ProcessStartInfo
            $info.FileName=$app.Exe; $info.Arguments=Get-MMGameArguments $app.Folder
            $info.UseShellExecute=$false; $info.CreateNoWindow=$true
            $app.Process=[Diagnostics.Process]::Start($info)
            [void][MMForeground]::AllowSetForegroundWindow($app.Process.Id)
            $app.Raised=$false
            $app.Window.FindName('Notice').Text=''
            Set-MMRunning $true
            $app.Timer.Start()
        } catch { [void](Show-MMMessage $_.Exception.Message 'MMModern could not start' 'OK' 'Error') }
    })
    $app.Timer.Add_Tick({
        $app=$script:MMState
        $process=$app.Process
        if (-not $process) { return }
        $process.Refresh()
        if (-not $process.HasExited) {
            if (-not $app.Raised) {
                $handle=Get-MMGameWindowHandle $process
                if ($handle -ne [IntPtr]::Zero) { $app.Raised=[MMForeground]::SetForegroundWindow($handle) }
            }
            return
        }
        $app.Timer.Stop(); $code=$process.ExitCode; $process.Dispose(); $app.Process=$null
        Set-MMRunning $false
        Update-MMReport
        [void]$app.Window.Activate(); [void]$app.Window.FindName('Play').Focus()
        if ($code -ne 0) {
            $log=[Environment]::ExpandEnvironmentVariables($app.Facts['log.file'])
            $tail=if (Test-Path -LiteralPath $log -PathType Leaf) { (Get-Content -LiteralPath $log -Encoding UTF8 -Tail 12) -join "`n" } else { 'The log could not be written.' }
            $choice=Show-MMMessage "The game exited with error $code.`n`n$tail`n`nOpen the log?" 'MMModern could not start' 'YesNo' 'Error'
            if ($choice -eq 'Yes' -and (Test-Path -LiteralPath $log -PathType Leaf)) { Start-Process notepad.exe -ArgumentList (ConvertTo-MMArgument $log) -WindowStyle Normal }
        }
    })
    $w.Add_Closing({ param($sender,$eventArgs) if ($script:MMState.Process) { $eventArgs.Cancel=$true } })
    $w.Add_ContentRendered({ [void]$script:MMState.Window.FindName('Play').Focus() })
    try { [void]$w.ShowDialog() } finally { $app.Timer.Stop() }
}
if ($MyInvocation.InvocationName -ne '.') { Start-MMLauncher }
