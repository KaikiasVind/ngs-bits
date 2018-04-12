
# Building ngs-bits from sources (Linux/MacOS)

## Dependencies

ngs-bits depends on the following software to be installed

* _g++_ (4.5 or higher)
* _qmake_ (Qt 5.5 or higher, including xmlpatterns and mysql package)
* _git_ (to extract the version hash)
* __optional:__ python and matplotlib (for plot generation in QC tools)

For example, the installation of the dependencies using Ubuntu 14.04/16.04 looks like that:

	> sudo apt-get install make g++ qt5-default libqt5xmlpatterns5-dev libqt5sql5-mysql git python python-matplotlib libbz2-dev liblzma-dev

## Download

Use git to clone the most recent release (the source code package of GitHub does not contains required sub-modules):

    git clone --recursive https://github.com/imgag/ngs-bits.git
	cd ngs-bits
	git checkout 2018_04
	git submodule update --recursive --init

### Resolving proxy issues with git

If you are behind a proxy that block the standard git port, you see something like this:

    $ git clone --recursive https://github.com/imgag/ngs-bits.git
    Cloning into 'ngs-bits'...
    fatal: Unable to look up github.com (port 9418) (Name or service not known)

Then you have to adapt your ~/.gitconfig file like that:

    [http]
    proxy = http://[user]:[password]@[host]:[port]


## Build

Just execute the following make commands:

    make build_3rdparty
	make build_tools_release

## Executing

Now the executables and all required libraries can be found in the `bin` folder.
