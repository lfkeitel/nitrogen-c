Nitrogen v0.1.0
================

Nitrogen is a Lisp-based programming language. Please note, this version is not complete and many things may change that can/will break other things. You have been warned.

Requirements for Building From Source
-------------------------------------

* gcc
* build-essential
* libedit-dev

To build the Nitrogen Interpreter
---------------------------------

1. git clone [https://github.com/dragonrider23/nitrogen](https://github.com/dragonrider23/nitrogen)
2. cd nitrogen
3. make
4. ./nitrogen

Language Documentation
----------------------

Language documentation coming soon but when available can be found at [http://onesimussystems.com/nitrogen](http://onesimussystems.com/nitrogen).

Versioning
----------

For transparency into the release cycle and in striving to maintain backward compatibility, This project is maintained under the Semantic Versioning guidelines. Sometimes we screw up, but we'll adhere to these rules whenever possible.

Releases will be numbered with the following format:

`<major>.<minor>.<patch>`

And constructed with the following guidelines:

- Breaking backward compatibility **bumps the major** while resetting minor and patch
- New additions without breaking backward compatibility **bumps the minor** while resetting the patch
- Bug fixes and misc changes **bumps only the patch**

For more information on SemVer, please visit <http://semver.org/>.
