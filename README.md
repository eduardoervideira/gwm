# gwm - gecko window manager

gwm is a window manager for X11 using XCB.

inspired by bspwm, dwm, ragnar and many other window managers.

# dependencies
todo

# installation
```bash
git clone
cd gwm
make config # copy default configuration file to config directory
sudo make install # install gwm to /usr/bin
```

# usage
```bash
# you can run gwm by adding it to your .xinitrc file:
exec gwm start
```

# configuration (gwmrc)
hot reload is not supported yet so gwm is configured by editing the gwmrc file located either on `~/.config/gwm/gwmrc` or `$XDG_CONFIG_HOME/gwm/gwmrc` (if the environment variable is set).  
the `gwmrc` file is divided into two sections: options and rules.  

> a better parser for the configuration file is still needed, so only the options available in the example configuration file can be used.

> rules are also not implemented yet, so they have no effect at this time.

# contributing
right now i'm not looking for contributors, but if you want to contribute, feel free to open a pull request.

# disclaimer
gwm is in early development and is not yet ready for daily use.  

this project is personal and the current version is a prototype, given that this idea is almost 1 year old.  
i finally decided to make it public to give me an incentive to continue development until it is ready for daily use.  
this version will undergo a major refactor, so some features may not work correctly and the code may not be the cleanest/most efficient.