/*
 * Copy me if you can.
 * by 20h
 */

#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
 * Get keyboard keys
 */

#include <X11/Xlib.h>

/*
 * Get username
 */

#include <pwd.h>

/*
 * Get Weather
 */

#include <curl/curl.h>

struct MemoryStruct {
	char *memory;
	size_t size;
};

/*
 * Get IPv4 address
 */

/* inet_ntoa */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/* ifreq */
#include <sys/ioctl.h>
#include <net/if.h>

/*
 * Get volume
 */
#include <alsa/asoundlib.h>
#include <alsa/control.h>


/*
 * Refresh Statusbar at time (currently not used)
 */
#include <X11/XF86keysym.h> /* billentyuzet funkciogombok */


/*
 * Set language
 */

#include <locale.h>


/* 
 * char *tzargentina = "America/Buenos_Aires"; 
 * char *tzutc = "UTC"; 
 */

char *tzberlin = "Europe/Budapest";

static Display *dpy;

char *fcbudapest = "Budapest";

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

void
settz(char *tzname)
{
	setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	/*Set language to Hungarian*/
	setlocale(LC_TIME, "hu_HU.UTF-8");

	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL)
		return smprintf("");

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		return smprintf("");
	}

	return smprintf("%s", buf);
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char *
loadavg(void)
{
	double avgs[3];

	if (getloadavg(avgs, 3) < 0)
		return smprintf("");

	return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

char *
readfile(char *base, char *file)
{
	char *path, line[513];
	FILE *fd;

	memset(line, 0, sizeof(line));

	path = smprintf("%s/%s", base, file);
	fd = fopen(path, "r");
	free(path);
	if (fd == NULL)
		return NULL;

	if (fgets(line, sizeof(line)-1, fd) == NULL) {
		fclose(fd);
		return NULL;
	}
	fclose(fd);

	return smprintf("%s", line);
}

char *
getbattery(char *base)
{
	char *co, status;
	int descap, remcap;

	descap = -1;
	remcap = -1;

	co = readfile(base, "present");
	if (co == NULL)
		return smprintf("");
	if (co[0] != '1') {
		free(co);
		return smprintf("not present");
	}
	free(co);

	co = readfile(base, "charge_full_design");
	if (co == NULL) {
		co = readfile(base, "energy_full_design");
		if (co == NULL)
			return smprintf("");
	}
	sscanf(co, "%d", &descap);
	free(co);

	co = readfile(base, "charge_now");
	if (co == NULL) {
		co = readfile(base, "energy_now");
		if (co == NULL)
			return smprintf("");
	}
	sscanf(co, "%d", &remcap);
	free(co);

	co = readfile(base, "status");
	if (!strncmp(co, "Discharging", 11)) {
		status = '-';
	} else if(!strncmp(co, "Charging", 8)) {
		status = '+';
	} else {
		status = '?';
	}

	if (remcap < 0 || descap < 0)
		return smprintf("invalid");

	return smprintf("%.0f%%%c", ((float)remcap / (float)descap) * 100, status);
}

char *
gettemperature(char *base, char *sensor)
{
	char *co;

	co = readfile(base, sensor);
	if (co == NULL)
		return smprintf("");
	return smprintf("%02.0f°C", atof(co) / 1000);
}

char *
execscript(char *cmd)
{
	FILE *fp;
	char retval[1025], *rv;

	memset(retval, 0, sizeof(retval));

	fp = popen(cmd, "r");
	if (fp == NULL)
		return smprintf("");

	rv = fgets(retval, sizeof(retval), fp);
	pclose(fp);
	if (rv == NULL)
		return smprintf("");
	retval[strlen(retval)-1] = '\0';

	return smprintf("%s", retval);
}

char *
getuser(void){
	/*
	 * Original from whoami.c by Richard Mlynarik.
	 */

	uid_t uid;
	uid_t NO_UID = -1;

	uid = geteuid();
	struct passwd *pw;
	pw = getpwuid(uid);

	if (uid == NO_UID || !pw)
		return smprintf("%s","nouser");

	char *username = pw->pw_name;

	return smprintf("%s",username);
}

char *
fgethostname(void)
{
	char host_buffer[256];
	int hostname;
	uid_t NO_hostname = -1;

	hostname = gethostname(host_buffer, sizeof(host_buffer));

	if (hostname == NO_hostname)
		return smprintf("%s","no hostname");

	return smprintf("%s",host_buffer);
}

char *
getipaddress(char *net_name)
{
	int n;
	struct ifreq ifr;
	struct sockaddr_in* addr;
	char *ip_address;
	uid_t NO_IP = -1;

	char *noip = "no ip";

	n = socket(AF_INET, SOCK_DGRAM, 0);

	if (n == NO_IP)
		return smprintf("%s", noip);

	ifr.ifr_addr.sa_family = AF_INET;


	strncpy(ifr.ifr_name, net_name, IFNAMSIZ-1);

	if (!(ioctl(n, SIOCGIFADDR, &ifr) == 0))
	{
		close(n);
		return smprintf("%s", noip);
	}
	close(n);

	addr = (struct sockaddr_in *)&ifr.ifr_addr;
			
	ip_address = inet_ntoa(addr->sin_addr);

	return smprintf("%s", ip_address);
}

size_t 
writeFunc(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	if(!ptr) {
		/* out of memory */
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize; 
}

char *
getweather(char *base)
{
	CURL *curl;
	CURLcode res;

	struct MemoryStruct chunk;

	chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
  	chunk.size = 0;    /* no data at this point */

	curl = curl_easy_init();

	if (!curl)
		return smprintf("%s", "Hiba: lib.");

	curl_easy_setopt(curl, CURLOPT_URL, base);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	
	res = curl_easy_perform(curl);

	if (res != CURLE_OK)
		return smprintf("%s", "Hiba: net.");

	curl_easy_cleanup(curl);

	return smprintf("%s",chunk.memory);
}

char *
getvol(void)
{
	int vol;
    snd_hctl_t *hctl;
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_value_t *control;

	/* 
	 * To find card and subdevice: /proc/asound/, aplay -L, amixer controls
	 */

    snd_hctl_open(&hctl, "hw:0", 0);
    snd_hctl_load(hctl);

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);

	/*
	 * amixer controls
	 */

    snd_ctl_elem_id_set_name(id, "Master Playback Volume");

    snd_hctl_elem_t *elem = snd_hctl_find_elem(hctl, id);

    snd_ctl_elem_value_alloca(&control);
    snd_ctl_elem_value_set_id(control, id);

    snd_hctl_elem_read(elem, control);
    vol = (int)snd_ctl_elem_value_get_integer(control,0);

    snd_hctl_close(hctl);
    return smprintf("%d", vol);
}

int
main(void)
{
	char *status;

	/*
	 * char *avgs;
	 * char *tmar;
	 * char *tmutc;
	 * char *kbmap;
	 * char *surfs;
	 *
	 * char *hostname;
	 */

	char *bat;
	char *tmbln;
	char *t0;
	char *t1;
	char *wifi_ipv4;
	char *eth_ipv4;
	char *users;
	char *weather;
	char *vol;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

	for (;;sleep(30)) {

		/*
		 * avgs = loadavg();
		 * tmar = mktimes("%H:%M", tzargentina);
		 * tmutc = mktimes("%H:%M", tzutc);
		 * kbmap = execscript("setxkbmap -query | grep layout | cut -d':' -f 2- | tr -d ' '");
		 * surfs = execscript("surf-status");
		 *
		 * hostname = fgethostname();
		 */

		bat = getbattery("/sys/class/power_supply/BAT1");
		tmbln = mktimes("%Y.%b.%d. %H:%M,%A", tzberlin);
		t0 = gettemperature("/sys/devices/virtual/thermal/thermal_zone0", "temp");
		t1 = gettemperature("/sys/devices/virtual/thermal/thermal_zone1", "temp");

		wifi_ipv4 = getipaddress("wlp4s0");
		eth_ipv4 = getipaddress("enp0s31f6");
		users = getuser();
		weather = getweather("https://wttr.in/Budapest?format=3");
		vol = getvol();

		/*
		 *status = smprintf("S:%s K:%s T:%s|%s L:%s B:%s A:%s U:%s %s",
		 *		surfs, kbmap, t0, t1, avgs, bat, tmar, tmutc,
		 *		tmbln);
		 */

		status = smprintf("| 🌡:%s|%s | 🔋:%s | 🕑:%s | ᯤ:%s,🖧:%s | %s | %s | %s |", 
				t0, t1, bat, tmbln, wifi_ipv4, eth_ipv4, weather, users, vol);
		setstatus(status);

		/*
		 * free(surfs);
		 * free(kbmap);
		 * free(avgs);
		 * free(tmar);
		 * free(tmutc);
		 *
		 * free(hostname);
		 */

		free(t0);
		free(t1);
		free(bat);
		free(tmbln);
		free(status);

		free(wifi_ipv4);
		free(eth_ipv4);
		free(users);
		free(weather);
		free(vol);
	}

	XCloseDisplay(dpy);

	return 0;
}