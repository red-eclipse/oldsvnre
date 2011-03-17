<?php
    $app['releasever'] = "v1.0 (Ides Edition)";
    $app['releasedate'] = "15th March 2011";
    $app['background'] = "/bits/background_01.jpg";
    $app['youtubevid'] = "RYXDg35dobc";
    $app['screenshots'] = 16;

    $app['banner'] = "Please help us raise <b>AU$294</b> for our new freely licensed player model!";
    $app['bannerurl'] = "/donate";

    $app['targets'] = array('home' => array('name' => '', 'url' => '/', 'alturl' => '', 'nav' => -1, 'redir' => 0));

    // nav items should be in reverse order for the top navbar
    $app['targets']['donate'] = array('name' => 'Donate', 'url' => 'https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=212900', 'alturl' => 'https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=212900', 'nav' => 1, 'redir' => 1);
    $app['targets']['chat'] = array('name' => 'Chat', 'url' => 'http://webchat.freenode.net/?channels=redeclipse', 'alturl' => '', 'nav' => 1, 'redir' => 1);
    $app['targets']['forum'] = array('name' => 'Forum', 'url' => 'http://forum.freegamedev.net/viewforum.php?f=53', 'alturl' => 'http://forum.freegamedev.net/viewforum.php?f=53&', 'nav' => 1, 'redir' => 1);
    $app['targets']['download'] = array('name' => 'Download', 'url' => 'http://sourceforge.net/projects/redeclipse/files/redeclipse_1.0/', 'alturl' => 'http://sourceforge.net/projects/redeclipse/files/redeclipse_1.0/', 'nav' => 1, 'redir' => 1);

    $app['targets']['project'] = array('name' => 'Project', 'url' => 'http://sourceforge.net/projects/redeclipse/', 'alturl' => '', 'nav' => -1, 'redir' => 1);
    $app['targets']['devel'] = array('name' => 'Devel', 'url' => 'http://sourceforge.net/scm/?type=svn&group_id=326559', 'alturl' => 'http://sourceforge.net/scm/?type=svn&group_id=326559', 'nav' => -1, 'redir' => 1);
    $app['targets']['svn'] = array('name' => 'SVN', 'url' => 'http://redeclipse.svn.sourceforge.net/redeclipse/', 'alturl' => 'http://redeclipse.svn.sourceforge.net/viewvc/redeclipse/?view=rev&rev=', 'nav' => -1, 'redir' => 1);

    $app['targets']['youtube'] = array('name' => 'youtube', 'url' => 'http://www.youtube.com/results?search_query=redeclipse', 'alturl' => 'http://www.youtube.com/results?search_query=redeclipse+', 'nav' => 0, 'redir' => 1);
    $app['targets']['google'] = array('name' => 'google', 'url' => 'http://www.google.com/search?q=%22Red%20Eclipse%22', 'alturl' => 'http://www.google.com/search?q=%22Red%20Eclipse%22+', 'nav' => 0, 'redir' => 1);
    //$app['targets']['playdeb'] = array('name' => 'playdeb', 'url' => 'http://www.playdeb.net/software/Red%20Eclipse', 'nav' => 0, 'redir' => 1);

    function checkarg($arg = "", $def = "") {
        return isset($_GET[$arg]) && $_GET[$arg] != "" ? $_GET[$arg] : $def;
    }
    
    $app['target'] = checkarg("target", "home");
    if (!isset($app['targets'][$app['target']])) $app['target'] = "home";

    $title = checkarg("title");
    if ($app['targets'][$app['target']]['redir']) {
        $app['url'] = $title != "" ? (
                $app['targets'][$app['target']]['alturl'] != "" ? $app['targets'][$app['target']]['alturl'].$title : $app['targets'][$app['target']]['url'].$title
        ) : $app['targets'][$app['target']]['url'];
        header("Location: ".$app['url']);
        exit;
    }
    else {
        $app['url'] = $title != "" ? (
                $app['targets'][$app['target']]['alturl'] != "" ? $app['targets'][$app['target']]['alturl'].$title : $app['targets'][$app['target']]['url'].$title
        ) : $app['targets'][$app['target']]['url'];
        $app['navbar'] = ''; // cache the navbar
        foreach ($app['targets'] as $key => $targ) {
            if ($key != "" && $targ['name'] != "" && $targ['nav'] == 1) {
                $app['navbar'] .= '<a href="/'.$key.'">'. $targ['name'] .'</a> ';
            }
        }
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en" dir="ltr">
    <head>
        <title>Red Eclipse: <?php echo ($app['targets'][$app['target']]['name'] != "" ? $app['targets'][$app['target']]['name'] : "Home"); ?></title>
        <meta http-equiv="Content-Type" content="text/html;charset=utf-8" />
        <link rel="shortcut icon" href="/bits/favicon.ico" />
        <link rel="stylesheet" type="text/css" href="/bits/style.css" />
        <script type="text/javascript" src="/bits/js/jquery.js"></script>
        <script type="text/javascript" src="/bits/js/jquery.lightbox-0.5.js"></script>
        <script type="text/javascript">
            $(function() {
                $('#gallery a').lightBox();
            });
        </script>
        <script type="text/javascript" src="/bits/js/easySlider1.7.js"></script>
        <script type="text/javascript">
            $(document).ready(function(){   
                $("#slider").easySlider({
                    auto:           true,
                    continuous:     true,
                    nextId:         "slider1next",
                    prevId:         "slider1prev",
                    prevText: 		"Previous",
                    nextText: 		"Next",
                    controlsShow:	true,
                    controlsBefore:	"",
                    controlsAfter:	"",	
                    controlsFade:	true,
                    firstId: 		"firstBtn",
                    firstText: 		"First",
                    firstShow:		false,
                    lastId: 		"lastBtn",	
                    lastText: 		"Last",
                    lastShow:		false,				
                    vertical:		false,
                    speed: 			500,
                    pause:			10000,
                    numeric: 		false,
                    numericId: 		"controls"
                });
            }); 
        </script>
    </head>
    <body style="background: url(/bits/bg2.jpg) no-repeat center top, #282828 url(/bits/bg1.jpg) repeat-x" bgcolor="#333">
        <div id="container">
            <div id="banner"><a href="<?php echo $app['bannerurl']; ?>"><?php echo $app['banner']; ?></a></div>
            <div id="links"><?php echo $app['navbar']; ?></div>
            <div id="header">
                <a href="/home"><img src="/bits/lightbox-blank.gif" alt="Red Eclipse" width="450" height="143" border="0" align="left" title="Red Eclipse" /></a><a href="http://www.cubeengine.com/"><img src="/bits/lightbox-blank.gif" alt="Built on Cube Engine 2" width="150" height="143" border="0" align="right" title="Built on Cube Engine 2" /></a>
            </div>
            <div id="video">
                <div id="main">
                    <h1>Red Eclipse</h1><h2>&nbsp;&nbsp;&nbsp;&nbsp;a Free and Open Source FPS</h2>
                    <h3>Available for Windows, Linux/BSD and Mac OSX</h3>
                    <h3>Agile gameplay: wall run/kick, impulse dash, and jetpack</h3>
                    <h3>Builtin Editor: create your own maps cooperatively online</h3>
                    <a href="/download" id="button">Free Download<br /><em><?php echo $app['releasever']; ?><br /> released <i><?php echo $app['releasedate']; ?></i></em></a>	 
                    <p id="svn">or <a href="/devel">get the SVN version</a> and live on the bleeding edge</p>
                </div>
                <div id="player">
                    <object width="500" height="308" type="application/x-shockwave-flash" data="http://www.youtube.com/v/<?php echo $app['youtubevid']; ?>&amp;color1=0x000000&amp;color2=0x000000&amp;border=0&amp;fs=1&amp;egm=0&amp;showsearch=0&amp;showinfo=0&amp;ap=%2526fmt%3D18">
                        <param name="movie" value="http://www.youtube.com/v/<?php echo $app['youtubevid']; ?>&amp;color1=0x000000&amp;color2=0x000000&amp;border=0&amp;fs=1&amp;egm=0&amp;showsearch=0&amp;showinfo=0&amp;ap=%2526fmt%3D18" />
                        <param name="allowscriptaccess" value="always" />
                        <param name="allowFullScreen" value="true" />
                    </object>
                </div>
            </div>
            <div class="endblock">&nbsp;</div>
            <div class="sliderblock">
                <div id="slider">
                    <ul id="gallery">
<?php                   $i = 1;
                        $c = true;
                        while ($app['screenshots'] >= $i) {
                            $j = $i%4;
                            if ($j == 1) {
                                echo "<li>";
                                $c = false;
                            }
                            $k = $i < 10 ? "0".$i."" : "".$i."";
                            echo "<a href=\"/bits/images/".$k.".jpg\"><img src=\"/bits/thumbs/".$k.".jpg\" width=\"184\" height=\"138\" border=\"0\" alt=\"Screenshot ".$k."\" /></a>";
                            if ($j == 4) {
                                echo "</li>";
                                $c = true;
                            }
                            $i++;
                        }
                        if ($c != true) { echo "</li>"; } ?>
                    </ul>
                </div>
            </div>
            <div class="endblock">&nbsp;</div>
            <div class="leftcol">
                <p>Red Eclipse is a <i>Free and Open Source</i> game, using <a href="http://libsdl.org/">SDL</a> and <a href="http://opengl.org/">OpenGL</a> which allows it to be ported to many platforms; <!--you can <a href="/download">download a package</a> for Windows, Linux/BSD, Mac OSX, or -->grab a development copy from our <a href="/devel">Subversion</a> repository and live on the bleeding edge.</p>
                <p>The game is a single-player and multi-player first-person ego-shooter, built as a total conversion of <a href="http://www.cubeengine.com/">Cube Engine 2</a>, which lends itself toward a balanced gameplay, completely at the control of map makers, while maintaining a general theme of agility in a variety of environments. For more information, please see our <a href="/chat">Chat</a>.</p>
            </div>
            <div class="vbar">&nbsp;</div>
            <div class="leftcol">
                <p>In a true open source <i>by the people for the people</i> nature, it tries to work closely with the gaming and open source communities to provide a better overall experience, taking all sorts of feedback from your average player, to your seasoned developer, aiming to create a story-driven game environment that is flexible, fun, easy to use, and pleasing to the eye.</p>
            </div>
            <div class="rightblock">
                <h4>Support Us</h4>
                <p id="donatemsg">Red Eclipse is developed by volunteers, and you get it free of charge; your donations go toward ongoing costs which keep this project alive.</p>
                <form action="https://www.paypal.com/cgi-bin/webscr" method="post">
                    <input type="hidden" name="cmd" value="_s-xclick" />
                    <input type="hidden" name="hosted_button_id" value="212900" />
                    <input type="image" src="https://www.paypal.com/en_AU/i/btn/btn_donate_LG.gif" name="submit" alt="Support Us - Donate" />
                    <img alt="Support Us - Donate" border="0" src="https://www.paypal.com/en_AU/i/scr/pixel.gif" width="1" height="1" />
                </form>
            </div>
            <div class="endrightblock">&nbsp;</div>
            <div class="rightblock">
                <ul>
<?php               foreach ($app['targets'] as $key => $targ) {
                        if ($key != "" && $targ['name'] != "" && $targ['nav'] == 0) {
                            echo "<li><a href=\"/". $key ."\" class='info'><span id=\"". $targ['name'] ."\">&nbsp;</span><span class=\"bubble\">on ". $targ['name'] ."</span></a></li>";
                        }
                    } ?>
                </ul>
            </div>
            <div class="endrightblock">&nbsp;</div>
            <div id="footer">
                <div id="sflogo">
                    <a href="/project">
                        <img src="http://sflogo.sourceforge.net/sflogo.php?group_id=326559&amp;type=8" alt="Get Red Eclipse at SourceForge" title="Get Red Eclipse at SourceForge" width="80" height="15" border="0" />
                    </a>
                </div>
                <a href="/download">Download</a>, or <a href="/chat">Talk to Us</a> today.
            </div>
            <div id="copyright" align="center">
                <p>Red Eclipse, Copyright (C) 2011 Quinton Reeves, Lee Salzman</p>
                <p>Cube Engine 2, Copyright (C) 2001-2011 Wouter van Oortmerssen, Lee Salzman, Mike Dysart, Robert Pointon, and Quinton Reeves</p>
            </div>
        </div>
    </body>
</html>
<?php } ?>
