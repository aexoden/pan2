import os

LAUNCHDIR=GetLaunchDir()
VARCACHE=os.path.join(LAUNCHDIR,'variables.cache.py')

vars=Variables([VARCACHE,os.path.join(LAUNCHDIR,'custom.py'),'custom.py'])
vars.AddVariables(PathVariable('BUILDDIR','Directory for built files.','#/../build',PathVariable.PathIsDirCreate),
                  PathVariable('PREFIX','Install prefix.','/usr/local',PathVariable.PathAccept)
                  )

AddOption('--with-gtkspell',dest='gtkspell',action='store_true',
          help='Enable use of GtkSpell.')

def awk_tool(env):
    awk=env.WhereIs('awk')
    if not awk:
        awk=WhereIs('gawk')
    if awk:
        env['AWK']=awk
    
tools=['default',awk_tool]
if os.name=='nt' :
    tools[0]='mingw'
#DefaultEnvironment(ENV=os.environ,tools=tools)
env=Environment(ENV=os.environ,variables=vars,tools=tools)
Help(vars.GenerateHelpText(env))
vars.Save(VARCACHE,env)

def PkgConfig(context,mod,minversion,extras='',maxversion=None):
    """Use pkg-config to test for module mod between minversion & max version.
    Update flags with cflags & ldflags for mod and additional modules extras."""
    if not maxversion :
        context.Message('Checking for %s %s ... '%(mod,minversion))
        ret=context.TryAction('pkg-config --atleast-version %s %s'%(minversion,mod))
    else:
        context.Message('Checking for %s %s-%s ... '%(mod,minversion,maxversion))
        ret=context.TryAction('pkg-config --atleast-version %s --max-version %s %s'
                              %(minversion,maxversion,mod))
    ret=ret[0]
    if isinstance(extras,list):
        extras=' '.join(extras)
    if ret:
        context.env.ParseConfig('pkg-config --cflags --libs %s %s'%(mod,extras))
    context.Result(ret)
    return ret

def Checkstrftime(context):
    """Does strftime support %l & %k."""
    context.Message('Checking for %l & %k support in strftime ...')
    ret=context.TryRun("""
#include <string.h>
#include <time.h>
int main(int argc, char **argv) {
  char buf[10];
  time_t rawtime = time(0);
  struct tm *timeinfo = localtime (&rawtime);
  strftime(buf, 10, "%l %k", timeinfo);
  exit (strstr(buf, "l") || strstr(buf, "k") || !strcmp(buf," "));
}
    """,'.c')
    context.Result(ret[0])
    #conf.Define('HAVE_LKSTRFTIME',1,'strftime supports l & k')
    key='HAVE_LKSTRFTIME'
    context.havedict[key]=ret[0];
    if ret[0]:
        line='#define %s 1\n'%key
    else:
        line='#undef %s\n'%key
    lines='\n/* Define if strftime supports l & k */\n'+line
    context.config_h=context.config_h + lines
    print dir(context)
    return ret[0]

def CheckGlibGettext(context):
    import SCons.Conftest as SC
    def glib_lc_messages():
        ret=SC.CheckHeader(context,'locale.h',language='C')
        if not ret:
            ret=SC.CheckDeclaration(context,'LC_MESSAGES','#include <locale.h>','C')
        return not ret
    def glib_with_nls():
        msgfmttext='''
msgid ""
msgstr ""
"Content-Type: text/plain; charset=UTF-8\n"
"Project-Id-Version: test 1.0\n"
"PO-Revision-Date: 2007-02-15 12:01+0100\n"
"Last-Translator: test <foo@bar.xx>\n"
"Language-Team: C <LL@li.org>\n"
"MIME-Version: 1.0\n"
"Content-Transfer-Encoding: 8bit\n"
'''
        havegt=False
        ret=not SC.CheckHeader(context,'libintl.h',language='C')
        if ret:
            dglibc=False
            btc=False
            dgintl=False
            libs=[]
            nglibc= not SC.CheckFunc(context,'ngettext')
            if nglibc:
                dglibc=not SC.CheckFunc(context,'dgettext')
                btc=not SC.CheckFunc(context,'bind_textdomain_codeset')
            if not nglibc or not dglibc or not btc:
                libs=['intl']
                if not SC.CheckLib(context,'intl','bindtextdomain',autoadd=False):
                    if not SC.CheckLib(context,'intl','ngettext',autoadd=False):
                        dgintl= not SC.CheckLib(context,'intl','dgettext',autoadd=False):
                if not dgintl:
                    if not SC.CheckLib(context,'intl','ngettext',extra_libs='iconv',autoadd=False):
                        if not SC.CheckLib(context,'intl','dgettext',extra_libs='iconv',autoadd=False):
                            dgintl=True
                            libs.extend(['iconv'])
                if dgintl:
                    if not btc:
                        oldlibs=context.AppendLIBS(libs)
                        btc=not SC.CheckFunc(context,'bind_textdomain_codeset')
                        context.SetLIBS(oldlibs)
                        #prefer libc if btc not found & both have ng and dg
                        if not btc and nglibc and dglibc:
                            dgintl=False
                if not dgintl:
                    libs=[]
            if dglibc or dgintl:
                havegt=True
            lines='\nDefine if the GNU gettext() function is already present or preinstalled.\n'
            if havegt:
                lines=lines+'#define HAVE_GETTEXT 1\n'
            else:
                lines=lines+'#undef HAVE_GETTEXT\n'
            context.config_h=context.config_h+lines
            if havegt:
                env=context.env
                if env.WhereIs('msgfmt'):
                    env['MSGFMT']='msgfmt'
                    env['MSGFMT_OPTS']=''
                    #use libs
                    oldlibs=context.AppendLIBS(libs)
                    SC.CheckFunc(context,'dcgettext')
                    #check msgfmt -c (msgfmt_opts)
                    context.Display('Does msgfmt support -c ... ')
                    (ret,fout)=context.TryAction('msgfmt -c -o '+os.devnull,msgfmttext)
                    if ret:
                        context.Display('yes\n')
                        env['MSGFMT_OPTS']='-c'
                    else:
                        context.Display('no\n')
                    if env.WhereIs('gmsgfmt'):
                        env['GMSGFMT']='gmsgfmt'
                    if env.WhereIs('xgettext'):
                        env['XGETTEXT']='xgettext'
                    ret=context.TryLink('''extern int _nl_msg_cat_cntr;
                        int main(char*,int){return _nl_msg_cat_cntr;}''','.c')
                    if ret:
                        env['CATOBJEXT']='.gmo'
                        env['DATADIRNAME']='share'
                    else:
                        env['CATOBJEXT']='.mo'
                        env['DATADIRNAME']='lib'
                        #no solaris with btc?
                        #yes gmo share
                        #no mo lib
                    #restore libs
                    context.SetLIBS(oldlibs)
                    env.['INSTOBJEXT']='.mo'
                else:
                    havegt=False
        context.AppendLibs(libs)
        lines='\n/*always defined to indicate that i18n is enabled.*/\n'
        if havegt:
            lines=lines+'#define ENABLE_NLS 1\n'
        else:
            lines=lines+'#undef ENABLE_NLS\n'
        context.config_h=context.config_h+lines
        if haskey(env,'XGETTEXT'):
            if not context.TryAction('$XGETTEXT --omit-header '+os.devnull,''):
                context.Dispaly('Found xgettext program is not GNU xgettext; ignore it.\n')
                del(env['XGETTEXT'])
        #process the po dir
   glib_lc_messages()
   glib_with_nls()

if not GetOption('help') and not GetOption('clean'):
    if not env.has_key('CXX') and env['CXX']!=None:
        print 'C++ compiler not found.'
        Exit(1)
    if not env.has_key('AR'):
        print 'ar not found.'
        Exit(1)
    if not env.has_key('RANLIB'):
        print 'ranlib not found.'
        Exit(1)
    if os.name=='nt' and not env.has_key('RC'):
        print 'resource compiler (windres) not found.'
        Exit(1)
    if not env.has_key('AWK'):
        print 'awk not found.'
        Exit(1)
    conf=Configure(env,config_h=env.subst('$BUILDDIR/config.h'),
                   log_file='$BUILDDIR/sc_conf.log',
                   conf_dir='$BUILDDIR/scons_conf',
                   custom_tests={'PkgConfig':PkgConfig,'Checkstrftime':Checkstrftime,
                    'CheckGlibGettext':CheckGlibGettext})
    conf.Define('VERSION_MAJOR',0,'Major part of version number.')
    conf.Define('VERSION_MINOR',133,'Minor part of version number.')
    conf.Define('VERSION_REVISION',0,'Revision part of version number.')
    conf.Define('VERSION_TITLE','x','Release name.')
    PCRE_REQUIRED='5.0'
    GLIB_REQUIRED='2.4.0'
    GMIME_REQUIRED='2.1.9'
    GTK_REQUIRED='2.4.0'
    GTKSPELL_REQUIRED='2.0.7'
    for h in ['stdio.h','stdlib.h','stddef.h','stdarg.h','string.h','float.h']:
        if not conf.CheckCHeader(h) :
            print 'Standard C header %s must be installed.'%(h)
            Exit(1)
    conf.Define('STDC_HEADERS',1,'Have Standard C headers.')
    if not conf.CheckCHeader('time.h'):
        print 'Header time.h must be installed.'
        Exit(1)
    conf.CheckFunc('localtime_r','#include <time.h>')
    if not conf.CheckHeader('tr1/unordered_set',language='C++'):
        print 'Header <tr1/unordered_set> must be installed.'
        Exit(1)
    conf.CheckHeader('ext/hash_set',language='C++')
    conf.CheckCHeader('errno.h')
    conf.CheckCHeader('fcntl.h')
    conf.CheckCHeader('malloc.h')
    conf.PkgConfig('glib-2.0',GLIB_REQUIRED)
    conf.PkgConfig('gtk+-2.0',GTK_REQUIRED,'gobject-2.0 gmodule-2.0 gthread-2.0')
    conf.PkgConfig('libpcre',PCRE_REQUIRED)
    conf.PkgConfig('gmime-2.0',GMIME_REQUIRED)
    if GetOption('gtkspell') and conf.PkgConfig('gtkspell-2.0',GTKSPELL_REQUIRED):
        conf.Define('HAVE_GTKSPELL',1,'Spellcheck library')
    conf.Checkstrftime()
    conf.Define('GETTEXT_PACKAGE','pan')
    conf.Define('PANLOCALDIR','locale')
    if os.name=='nt':
        env.MergeFlags('-mms-bitfields -mwin32 -mwindows -DWIN32_LEAN_AND_MEAN -lshell32 -lws2_32')
        conf.Define('HAVE_WIN32','1')
    conf.CheckGlibGettext()
    env=conf.Finish()

#print env['PLATFORM']
#print env['TOOLS']
#print env['ENV']['PATH']

env.AppendUnique(CPPPATH='#$BUILDDIR')
env.AppendUnique(CPPPATH='#/')
env.AppendUnique(CPPPATH='.')
env.AppendUnique(CPPDEFINES='HAVE_CONFIG_H')

#SConscript('SConscript',variant_dir=env['BUILDDIR'],duplicate=0,exports='env')
