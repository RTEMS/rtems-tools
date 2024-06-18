RTEMS Tools Project
===================

Chris Johns <chrisj@rtems.org>

Available on our GitLab  https://gitlab.rtems.org/rtems/tools/rtems-tools/

The RTEMS Tools Project is a collection of tools to help you use RTEMS. The
package is self contained and if in a release package format is specific to an
RTEMS release and if in the git repo is a development version.

All tools are distributed as source code. They should work on a range of host
computers. Windows support may be via cross building on suitable Unix systems.

The tools contained in this package each come with documentation so please
locate and refer to that.

The RTEMS Tools Project has been developed for the RTEMS Project however these
tools can be used for a range of things not related to RTEMS. The RTEMS Project
welcomes this.

If you have a problem or questions join us on our Discord at
https://www.rtems.org/discord and post in #general  You are welcome to come and 
chat to share how you use these tools as well.

If you have any patches please post them to the devel@rtems.org mailing list in
git format patches with your details.


Building
--------

To build and install:

```
  # ./waf configure --prefix=$HOME/development/rtems/5
  # ./waf build install
```

Testing
-------

To run the tests:

```
  # ./waf configure
  # pytest -v
```

These tests have only been verified to work with Python 3


Python
------

The RTEMS Tools supports python3 and python2. The commands look for python3,
then python2 and finally python and use the first it finds.

You can forced a specific version for testing by setting the environment
variable 'RTEMS_PYTHON_OVERRIDE' to the python you want to use. For example:

 $ export RTEMS_PYTHON_OVERRIDE=python2

will use python2.


Waf
---

The Waf project can be found here:

  * https://waf.io/
