# How to contribute

- Fork the repository on GitHub

- Create a topic branch from where you want to base your work

- When using otl   
    `sudo apt-get install unixODBC unixODBC-dev` or   
    `yum install unixODBC unixODBC-devel`  

## Adding functionality

- use STL whenever possible 

- icons and bitmaps
  - menu and toolbar bitmaps are from wxWidgets, using wxArtProvider

  - application icons from:    
  [IconFinder](https://www.iconfinder.com/icons/1495216/article_circle_edit_paper_pencil_icon#size=128),   
  [converted to ico](http://www.convertico.com/),   
  [converted to xpm using GIMP](http://www.gimp.org/),   
  [convert to mac icns (first make 128 by 128 icon)](http://iconverticons.com/)

- doxy document sources  
  API documentation is generated from the sources 
  using [Doxygen](http://www.stack.nl/~dimitri/doxygen/)
  
- add a test and update sample
 
- translation is done using xgettext by invoking po-sync.sh   
  - to translate wxExtension copy the wxex.pot file to the correct language po 
    file, and fill in the translation.

  - The place where to put your po files can be found by adding -l command line switch.
    You can also test other languages using the special LANG config item,
    e.g. setting it to french allows you to test french translation.
    Normally you check the current locale by running locale on the
    command line. Running locale -a shows all your available locales, if your
    locale is not present, add it using locale-gen (provided it is in 
    the list of all locales (/usr/share/i18n/SUPPORTED).
    Then you do export LANG=..., or change the /etc/default/locale.
    
  - To add translation files add -DENABLE_GETTEXT=ON to cmake.
    
## Test it

```
mkdir build && cd build   
cmake -DwxExBUILD_TESTS=ON ..   
make   
make test   
```

- test coverage can be generated by using `CMAKE_BUILD_TYPE=Coverage`    
  This requires `lcov` and `genhtml` tools (part of lcov, v1.9).    

```
mkdir Coverage && cd Coverage
cmake -DCMAKE_BUILD_TYPE=Coverage -DwxExBUILD_TESTS=ON ..   
make  
make lcov-prep  
make test  
make lcov  
```

  And, if you want to upload results to coveralls.    
  `make lcov-coveralls`   
