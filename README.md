# A Tiny Win32-based Launcher for FFXIV

This is a custom launcher for FFXIV. It primarily exists because the 
official launcher is heavy, and doesn't support saving your password.
Unlike other custom launchers, this one has no runtime dependencies
other than Windows itself, and it works perfectly in Wine.

## Features

- Can install itself to replace `ffxivboot.exe`, allowing it to be
launched from Steam.
- Supports both Steam and non-Steam accounts. If using a Steam account,
you must launch it from  Steam, as per the official launcher.
- Supports One-Time Passwords. In fact, I highly recommend using them.
You shouldn't just enter your Square Enix password into a random app
from the internet! With One-Time Passwords, this is significantly less
dubious.
- Supports saving your Username and Password.
  - If using One-Time Passwords, a tiny window will appear just asking
  for your One-Time Password.
- Zero-interaction launch (only if not using One-Time Password).
- Works completely in Linux via Wine. Since it replaces `ffxivboot.exe`,
you don't need to change any of your infrastructure in Linux to support
it. Your existing game manager (like Lutris, or Steam) won't need any 
changes or hacks!
- Less than 900kb.
- Detects when game is out of date, and will run the official launcher
for patching. Once patching is complete, it'll reinstall itself.
