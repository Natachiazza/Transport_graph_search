#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <sstream>
#include <climits>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
const int INF = INT_MAX;

class Time {
private:
    int minutes;

public:
    Time(): minutes(0) {}
    Time(int mins): minutes(mins) {}
    Time(int day, int hour, int minute) {
        minutes = day * 24 * 60 + hour * 60 + minute;
    }

    int getMinutes() const { return minutes;}
    int getMinute() const { return minutes % 60;}
    int getHour() const { return (minutes % (24 * 60)) / 60;}
    int getDay() const { return (minutes / (24 * 60)) % 7;}

    Time addMinutes(int mins) const {
        return Time(minutes + mins);
    }

    string dayName() const {
        const char* days[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"};
        return days[getDay()];
    }

    string toString() const {
        char buf[6];
        int h = getHour();
        int m = getMinute();
        buf[0] = '0' + (h/10);
        buf[1] = '0' + (h%10);
        buf[2] = ':';
        buf[3] = '0' + (m/10);
        buf[4] = '0' + (m%10);
        buf[5] = '\0';
        return string(buf);
    }

    string fullString() const {
        return dayName() + " " + toString();
    }

    bool operator<(const Time& other) const { 
        return minutes < other.minutes; }
    bool operator==(const Time& other) const { 
        return minutes == other.minutes; }
};

class Edge {
public:
    int id;
    int from;
    int to;
    int transport;
    int nameId;
    string days; 
    Time startTime;
    int duration;
    int price;

    Edge(int i, int f, int t, int tr, int nid, const string& d,
        const Time& start, int dur, int pr) : 
        id(i), from(f), to(t), transport(tr), nameId(nid),
        days(d), startTime(start), duration(dur), price(pr) {}

    pair<Time, int> nextDeparture(const Time& cur) const { //arr[v]
        const int PERIOD = 7*24*60;
        int curMins = cur.getMinutes();
        int base = (curMins / PERIOD) * PERIOD; //начало недели 

        int best = INF;
        for (int day = 0; day < 7; ++day) {
            if (days[day] == '1') {
                int dep = base + day * 24 * 60 + startTime.getMinutes();
                if (dep < curMins)
                    dep += PERIOD;
                if (dep < best)
                    best = dep;
            }
        }

        if (best == INF)
            return {Time(INF), INF};
        return {Time(best), best - curMins};
    }
};

class Path {
private:
    vector<int> edgeIds;
    vector<Time> departs;
    vector<Time> arrivals;
    vector<int> waits;
    int totalCost;

public:
    Path(): totalCost(0) {}

    void addEdge(int eid, const Time& dep, const Time& arr, int wait, int cost)
    {
        edgeIds.push_back(eid);
        departs.push_back(dep);
        arrivals.push_back(arr);
        waits.push_back(wait);
        totalCost += cost;
    }

    bool empty() const { return edgeIds.empty(); }

    int size() const { 
        return edgeIds.size(); }

    int edgeId(int idx) const { return edgeIds[idx]; }
    Time getDeparture(int idx) const { return departs[idx];}
    Time getArrival(int idx) const { return arrivals[idx]; }
    int getWait(int idx) const { return waits[idx]; }

    int getTotalCost() const { return totalCost; }

    int getTotalTime() const {
        if (arrivals.empty() || departs.empty()) return 0;
        return arrivals.back().getMinutes() - departs.front().getMinutes();
    }
};

class Graph {
private:
    vector<Edge> edges;
    vector<vector<int> > adj;
    unordered_map<string, int> cityId;
    unordered_map<string, int> transpId;
    unordered_map<string, int> nameId;
    vector<string> cityNames;
    vector<string> transpNames;
    vector<string> cruiseNames;

    int getCityId(const string& name) {
        if (cityId.count(name)) return cityId[name];
        int id = cityNames.size();
        cityId[name] = id;
        cityNames.push_back(name);
        adj.resize(id + 1);
        return id;
    }

    int getTranspId(const string& name) {
        if (transpId.count(name)) return transpId[name];
        int id = transpNames.size();
        transpId[name] = id;
        transpNames.push_back(name);
        return id;
    }

    int getNameId(const string& name) {
        if (nameId.count(name)) return nameId[name];
        int id = cruiseNames.size();
        nameId[name] = id;
        cruiseNames.push_back(name);
        return id;
    }

public:
    Graph() {}

 bool load(const string& filename) {
    ifstream file(filename);
    if (!file) {
        cerr << "Не открывается:" << filename << endl;
        return false;
    }

    string line;
    int lineNum = 0;
    string curFrom, curTo, curTransp;
    int curFromId = -1, curToId = -1, curTranspId = -1;
    bool reading = false;

    while (getline(file, line)) {
        lineNum++;

        size_t comment = line.find('#');
        if (comment != string::npos)
            line = line.substr(0, comment);

        size_t first = line.find_first_not_of(" \t");
        if (first == string::npos) {
            reading = false;
            continue;
        }
        size_t last = line.find_last_not_of(" \t");
        line = line.substr(first, last - first + 1);

        if (line[0] == '"') {
            vector<string> header;
            bool inKavich = false;
            string name;
            for (char c : line) {
                if (c == '"') {
                    if (!inKavich) {
                        inKavich = true;
                        name.clear();
                    } else {
                        inKavich = false;
                        header.push_back(name);
                    }
                } else if (inKavich) {
                    name += c;
                }
            }
            if (header.size() != 3) {
                cerr << lineNum << "неверный формат, ожидается 3 имени "<< endl;
                continue;
            }

            curFrom = header[0];
            curTo = header[1];
            curTransp = header[2];

            curFromId = getCityId(curFrom);
            curToId = getCityId(curTo);
            curTranspId = getTranspId(curTransp);

            reading = true; 
        }
        else if (reading && line[0] == '{') {
            if (line.back() != '}') {
                cerr << lineNum << "- нет закрывающей скобки" << endl;
                continue;
            }

            string inside = line.substr(1, line.size() - 2);

            first = inside.find_first_not_of(" \t");
            if (first == string::npos) continue;
            last = inside.find_last_not_of(" \t");
            inside = inside.substr(first, last - first + 1);

            stringstream ss(inside);
            string cruiseName, daysStr, timeStr;
            int duration, price;

            if (ss.peek() == '"') {
                ss.get();
                getline(ss, cruiseName, '"');
            } else {
                ss >> cruiseName;
            }

            if (!(ss >> daysStr >> timeStr >> duration >> price)) {
                cerr << lineNum << "Ошибка в записи рейса" << endl;
                continue;
            }

            if (daysStr.length() != 7) {
                cerr << lineNum << "Ошибка. Формат дней: 1111111" << endl;
                continue;
            }
            
            //00:00 
            int hour = 0, minute = 0;
            char colon;
            stringstream ts(timeStr);
            if (!(ts >> hour >> colon >> minute) || colon != ':') {
                cerr << lineNum << "неверный формат времени" << endl;
                continue;
            }
            Time start(0, hour, minute);

            int nameId = getNameId(cruiseName);
            int eid = edges.size();
            edges.push_back(Edge(eid, curFromId, curToId, curTranspId, 
                nameId, daysStr, start, duration, price));
            adj[curFromId].push_back(eid);
        }
        else {
            reading = false;
        }
    }
    return true;
}

    int transpCount() const { return transpNames.size(); }
    int cityCount() const { return cityNames.size(); }

    string getCityName(int id) const {
        if (id >= 0 && id < (int)cityNames.size())
            return cityNames[id];
        return "";}

    string getTranspName(int id) const {
        if (id >= 0 && id < (int)transpNames.size())
            return transpNames[id];
        return "";
    }

    string getCruiseName(int id) const {
        if (id >= 0 && id < (int)cruiseNames.size())
            return cruiseNames[id];
        return "";}

    const std::vector<Edge>& getAllEdges() const {
        return edges;
    }

    const vector<int>& getEdgesFrom(int city) const {
        static vector<int> empty;
        if (city >= 0 && city < (int)adj.size())
            return adj[city];
        return empty;
    }

    const Edge& getEdge(int eid) const { return edges[eid]; }

    int findCity(const string& name) const {
        for (size_t i = 0; i < cityNames.size(); ++i)
            if (cityNames[i] == name) return i;
        return -1;
    }

    vector<Edge> getEdgesBetween(int from, int to) const {
        vector<Edge> res;
        if (from < 0 || from >= (int)adj.size()) return res;
        for (size_t i = 0; i < adj[from].size(); ++i) {
            int eid = adj[from][i];
            const Edge& e = edges[eid];
            if (e.to == to) res.push_back(e);
        }
        // сортируем по времени отправления
        for (size_t i = 0; i < res.size(); ++i) {
            for (size_t j = i + 1; j < res.size(); ++j) {
                if (res[j].startTime.getMinutes() < res[i].startTime.getMinutes()) {
                    Edge tmp = res[i];
                    res[i] = res[j];
                    res[j] = tmp;
                }
            }
        }
        return res;
    }
};

unordered_set<int> parseTransportIds(const string& input) {
    unordered_set<int> result;
    stringstream ss(input);
    string t;

    while (getline(ss, t, ',')) {
        size_t start = t.find_first_not_of(" \t");
        if (start == string::npos) continue;
        size_t end = t.find_last_not_of(" \t");
        t = t.substr(start, end - start + 1);

        if (t.empty()) continue;

        int num;
        stringstream converter(t);
        if (converter >> num && num >= 0) {
            result.insert(num);
        }
    }
    return result;
}

unordered_set<int> buildAllowedSet(const string& input, int mode, const Graph& g) {
    if (mode== 3) 
        return unordered_set<int>();

    unordered_set<int> parsed = parseTransportIds(input);

    if (mode == 1) {
        return parsed;
    }
    else {
        unordered_set<int> all;
        for (int i = 0; i < g.transpCount(); ++i)
            all.insert(i);
        for (int id : parsed)
            all.erase(id);
        return all;
    }
}

Path findShortestTime(const Graph& g, int start, int target,
                    const unordered_set<int>& allowed, const Time& startTime)
{
    int n = g.cityCount();
    vector<int> d(n, INF); 
    vector<int> costD(n, INF);
    vector<bool> a(n, false);
    vector<int> prevEdge(n, -1);
    vector<Time> arrival(n, Time(INF));

    d[start] = 0;
    costD[start] = 0;
    arrival[start] = startTime;
    
    for (int iter = 0; iter < n; ++iter) {
        //1.
        int v = -1;
        int best = INF;
        for (int u = 0; u < n; ++u) {
            if (!a[u] && d[u] < best) {
                best = d[u];
                v = u;
            }
        }
        if (v == -1) break;
        a[v] = true;
        if (v == target) break;

        //2
        const vector<int>& eids = g.getEdgesFrom(v);
        for (size_t i = 0; i < eids.size(); ++i) {
            int eid = eids[i];
            const Edge& e = g.getEdge(eid);

            if (!allowed.empty() && allowed.find(e.transport) == allowed.end())
                continue;

            pair<Time, int> next = e.nextDeparture(arrival[v]);
            Time dep = next.first;//отпр e
            int wait = next.second;
            if (dep.getMinutes() == INF) continue;//нет рейсов

            Time arr = dep.addMinutes(e.duration);
            int newTime = d[v] + wait + e.duration;
            int newCost = costD[v] + e.price;
            int to = e.to;
            //3
            if (newTime < d[to] || (newTime == d[to] && newCost < costD[to]))
            {
                d[to] = newTime;
                costD[to] = newCost;
                prevEdge[to] = eid;
                arrival[to] = arr;
            }
        }
    }

    if (d[target] == INF)
        return Path();

    // восстановление пути
    vector<int> pathEids;
    int cur = target;
    while (cur != start) {
        int eid = prevEdge[cur];
        if (eid == -1) return Path();
        pathEids.push_back(eid);
        cur = g.getEdge(eid).from;
    }
    reverse(pathEids.begin(), pathEids.end());

    Path result;
    Time curTime = startTime;
    for (size_t i = 0; i < pathEids.size(); ++i) {
        int eid = pathEids[i];
        const Edge& e = g.getEdge(eid);
        pair<Time, int> next = e.nextDeparture(curTime);
        Time dep = next.first;
        int wait = next.second;
        Time arr = dep.addMinutes(e.duration);
        result.addEdge(eid, dep, arr, wait, e.price);
        curTime = arr;
    }
    return result;
}

Path findCheapest(const Graph& g, int start, int target, const unordered_set<int>& allowed,
                  const Time& startTime)
{
    int n = g.cityCount();
    vector<int> d(n, INF); // стоимость
    vector<int> timeD(n, INF); // время при этой стоимости
    vector<bool> a(n, false);
    vector<int> prevEdge(n, -1);
    vector<Time> arrival(n, Time(INF));

    d[start] = 0;
    timeD[start] = 0;
    arrival[start] = startTime;
    //1
    for (int iter = 0; iter < n; ++iter) {
        int v = -1;
        int best = INF;
        for (int u = 0; u < n; ++u) {
            if (!a[u] && d[u] < best) {
                best = d[u];
                v = u;
            }
        }
        if (v == -1) break;
        a[v] = true;
        if (v == target) break;
        //2
        const vector<int>& eids = g.getEdgesFrom(v);
        for (size_t i = 0; i < eids.size(); ++i) {
            int eid = eids[i];
            const Edge& e = g.getEdge(eid);

            if (!allowed.empty() && allowed.find(e.transport) == allowed.end())
                continue;

            pair<Time, int> next = e.nextDeparture(arrival[v]);
            Time dep = next.first;
            int wait = next.second;
            if (dep.getMinutes() == INF) continue;

            Time arr = dep.addMinutes(e.duration);
            int newCost = d[v] + e.price;
            int newTime = timeD[v] + wait + e.duration;
            int to = e.to;

            if (newCost < d[to] ||
                (newCost == d[to] && newTime < timeD[to]))
            {
                d[to] = newCost;
                timeD[to] = newTime;
                prevEdge[to] = eid;
                arrival[to] = arr;
            }
        }
    }

    if (d[target] == INF) return Path();

    vector<int> pathEids;
    int cur = target;
    while (cur != start) {
        int eid = prevEdge[cur];
        if (eid == -1) return Path();
        pathEids.push_back(eid);
        cur = g.getEdge(eid).from;
    }
    reverse(pathEids.begin(), pathEids.end());

    Path result;
    Time curTime = startTime;
    for (size_t i = 0; i < pathEids.size(); ++i) {
        int eid = pathEids[i];
        const Edge& e = g.getEdge(eid);
        pair<Time, int> next = e.nextDeparture(curTime);
        Time dep = next.first;
        int wait = next.second;
        Time arr = dep.addMinutes(e.duration);
        result.addEdge(eid, dep, arr, wait, e.price);
        curTime = arr;
    }
    return result;
}

Path findMinTransfers(const Graph& g, int start, int target, 
                      const unordered_set<int>& allowed, const Time& startTime)
{
    int n = g.cityCount();

    vector<int> d(n, INF);
    vector<bool> a(n, false);
    vector<int> prevEdge(n, -1); 
    queue<int> q; 

    d[start] = 0;
    a[start] = true;
    q.push(start);

    while (!q.empty()) {
        int v = q.front();
        q.pop();
        if (v == target) break; 

        const vector<int>& eids = g.getEdgesFrom(v);
        for (size_t i = 0; i < eids.size(); ++i) {
            int eid = eids[i];
            const Edge& e = g.getEdge(eid);

            if (!allowed.empty() && allowed.find(e.transport) == allowed.end())
                continue;

            int to = e.to;
            if (!a[to]) {
                a[to] = true;
                d[to] = d[v] + 1;
                prevEdge[to] = eid;
                q.push(to);
            }
        }
    }

    if (!a[target]) return Path();  

    vector<int> pathEids;
    int cur = target;
    while (cur != start) {
        int eid = prevEdge[cur];
        if (eid == -1) return Path();
        pathEids.push_back(eid);
        cur = g.getEdge(eid).from;
    }
    reverse(pathEids.begin(), pathEids.end());

    Path result;
    Time curTime = startTime;
    for (size_t i = 0; i < pathEids.size(); ++i) {
        int eid = pathEids[i];
        const Edge& e = g.getEdge(eid);
        pair<Time, int> next = e.nextDeparture(curTime);
        Time dep = next.first;
        int wait = next.second;
        Time arr = dep.addMinutes(e.duration);
        result.addEdge(eid, dep, arr, wait, e.price);
        curTime = arr;
    }
    return result;
}

vector<pair<int, Path> > findWithinBudget(const Graph& g,int start, int budget,
                                          const unordered_set<int>& allowed,
                                          const Time& startTime)
{
    int n = g.cityCount();
    vector<int> d(n, INF);
    vector<int> timeD(n, INF);
    vector<bool> a(n, false);
    vector<int> prevEdge(n, -1);
    vector<Time> arrival(n, Time(INF));

    d[start] = 0;
    timeD[start] = 0;
    arrival[start] = startTime;

    for (int iter = 0; iter < n; ++iter) {
        int v = -1;
        int best = INF;
        for (int u = 0; u < n; ++u) {
            if (!a[u] && d[u] < best) {
                best = d[u];
                v = u;
            }
        }
        if (v == -1) break;
        a[v] = true;

        const vector<int>& eids = g.getEdgesFrom(v);
        for (size_t i = 0; i < eids.size(); ++i) {
            int eid = eids[i];
            const Edge& e = g.getEdge(eid);
            if (!allowed.empty() && allowed.find(e.transport) == allowed.end())
                continue;

            pair<Time, int> next = e.nextDeparture(arrival[v]);
            Time dep = next.first;
            int wait = next.second;
            if (dep.getMinutes() == INF) continue;

            int newCost = d[v] + e.price;
            if (newCost > budget) continue;

            Time arr = dep.addMinutes(e.duration);
            int newTime = timeD[v] + wait + e.duration;
            int to = e.to;

            if (newCost < d[to] ||
                (newCost == d[to] && newTime < timeD[to]))
            {
                d[to] = newCost;
                timeD[to] = newTime;
                prevEdge[to] = eid;
                arrival[to] = arr;
            }
        }
    }

    vector<pair<int, Path> > result;
    for (int city = 0; city < n; ++city) {
        if (city == start) continue;
        if (d[city] <= budget && d[city] != INF) {
            vector<int> pathEids;
            int cur = city;
            while (cur != start) {
                int eid = prevEdge[cur];
                if (eid == -1) break;
                pathEids.push_back(eid);
                cur = g.getEdge(eid).from;
            }
            reverse(pathEids.begin(), pathEids.end());

            Path p;
            Time curTime = startTime;
            for (size_t i = 0; i < pathEids.size(); ++i) {
                int eid = pathEids[i];
                const Edge& e = g.getEdge(eid);
                pair<Time, int> next = e.nextDeparture(curTime);
                Time dep = next.first;
                int wait = next.second;
                Time arr = dep.addMinutes(e.duration);
                p.addEdge(eid, dep, arr, wait, e.price);
                curTime = arr;
            }
            result.push_back(make_pair(city, p));
        }
    }
    return result;
}

vector<pair<int, Path> > findWithinTime(const Graph& g,
                                        int start,
                                        int timeLimit,
                                        const unordered_set<int>& allowed,
                                        const Time& startTime)
{
    int n = g.cityCount();
    vector<int> d(n, INF);// время
    vector<int> costD(n, INF);
    vector<bool> a(n, false);
    vector<int> prevEdge(n, -1);
    vector<Time> arrival(n, Time(INF));

    d[start] = 0;
    costD[start] = 0;
    arrival[start] = startTime;

    for (int iter = 0; iter < n; ++iter) {
        int v = -1;
        int best = INF;
        for (int u = 0; u < n; ++u) {
            if (!a[u] && d[u] < best) {
                best = d[u];
                v = u;
            }
        }
        if (v == -1) break;
        a[v] = true;

        const vector<int>& eids = g.getEdgesFrom(v);
        for (size_t i = 0; i < eids.size(); ++i) {
            int eid = eids[i];
            const Edge& e = g.getEdge(eid);
            if (!allowed.empty() && allowed.find(e.transport) == allowed.end())
                continue;

            pair<Time, int> next = e.nextDeparture(arrival[v]);
            Time dep = next.first;
            int wait = next.second;
            if (dep.getMinutes() == INF) continue;

            int newTime = d[v] + wait + e.duration;
            if (newTime > timeLimit) continue;

            Time arr = dep.addMinutes(e.duration);
            int newCost = costD[v] + e.price;
            int to = e.to;

            if (newTime < d[to] ||
                (newTime == d[to] && newCost < costD[to]))
            {
                d[to] = newTime;
                costD[to] = newCost;
                prevEdge[to] = eid;
                arrival[to] = arr;
            }
        }
    }

    vector<pair<int, Path> > result;
    for (int city = 0; city < n; ++city) {
        if (city == start) continue;
        if (d[city] <= timeLimit && d[city] != INF) {
            vector<int> pathEids;
            int cur = city;
            while (cur != start) {
                int eid = prevEdge[cur];
                if (eid == -1) break;
                pathEids.push_back(eid);
                cur = g.getEdge(eid).from;
            }
            reverse(pathEids.begin(), pathEids.end());

            Path p;
            Time curTime = startTime;
            for (size_t i = 0; i < pathEids.size(); ++i) {
                int eid = pathEids[i];
                const Edge& e = g.getEdge(eid);
                pair<Time, int> next = e.nextDeparture(curTime);
                Time dep = next.first;
                int wait = next.second;
                Time arr = dep.addMinutes(e.duration);
                p.addEdge(eid, dep, arr, wait, e.price);
                curTime = arr;
            }
            result.push_back(make_pair(city, p));
        }
    }
    return result;
}

class UI {
private:
    Graph g;

    string inputLine() {
        string s;
        getline(cin, s);
        return s;}

    void printHeader(const string& s) {
        cout << "\n" << string(60, '=') << "\n";
        cout << s << "\n";
        cout << string(60, '=') << "\n";
    }

    void showCities() {
        printHeader("СПИСОК ГОРОДОВ");
        int n = g.cityCount();
        if (n == 0) {
            cout << "Нет загруженных городов.\n";
            return;
        }
        for (int i = 0; i < n; ++i) {
            cout << i << ". " << g.getCityName(i) << "\n";
        }
        cout << "Всего: " << n << "\n";
    }

    void showTransport() {
        printHeader("ТИПЫ ТРАНСПОРТА");
        int n = g.transpCount();
        if (n == 0) {
            cout << "Нет данных.\n";
            return;
        }
        for (int i = 0; i < n; ++i) {
            cout << i << ". " << g.getTranspName(i) << "\n";
        }
    }

    void showSchedule() {
        printHeader("РАСПИСАНИЕ");
        cout << "Город отправления: ";
        string fromName = inputLine();
        int from = g.findCity(fromName);
        if (from == -1) {
            cout << "Город не найден.\n";
            return;
        }
        cout << "Город назначения: ";
        string toName = inputLine();
        int to = g.findCity(toName);
        if (to == -1) {
            cout << "Город не найден.\n";
            return;
        }

        vector<Edge> edges = g.getEdgesBetween(from, to);
        if (edges.empty()) {
            cout << "Нет прямых рейсов.\n";
            return;
        }

        cout << "\nРейсы " << fromName << " -> " << toName << ":\n";
        for (size_t i = 0; i < edges.size(); ++i) {
            const Edge& e = edges[i];
            cout << i+1 << ". " << g.getTranspName(e.transport)
                 << " \"" << g.getCruiseName(e.nameId) << "\"\n";
            cout << "   Отправление " << e.startTime.toString()
                 << "  В пути " << e.duration << " мин"
                 << "  Цена " << e.price << "р\n";
            cout << "   Дни: ";
            for (int d = 0; d < 7; ++d) {
                if (e.days[d] == '1') {
                    const char* names[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"};
                    cout << names[d] << " ";
                } else {
                    cout << "-- ";
                }
            }
            cout << "\n\n";
        }
    }

    Time getStartTime() {
        cout << "День недели (0-6, Пн=0-Вс=6): ";
        string ds = inputLine();
        int day = 0;
        if (!ds.empty()) day = stoi(ds) % 7;
        cout << "Время (чч:мм): ";
        string ts = inputLine();
        int h = 9, m = 0;
        size_t col = ts.find(':');
        if (col != string::npos) {
            h = stoi(ts.substr(0, col));
            m = stoi(ts.substr(col+1));
        }
        return Time(day, h, m);
    }

    void search(int mode) {
        cout << "\nОграничения на транспорт:\n";
        cout << "1. Только ..\n";
        cout << "2. Все кроме..\n";
        cout << "3. Все\n";
        cout << "Выбор: ";
        string ch = inputLine();
        int transpMode = 3;
        if (!ch.empty()) transpMode = stoi(ch);

        unordered_set<int> allowedSet;
        if (transpMode != 3) {
            cout << "Введите ID типов транспорта через запятую ";
            string transps = inputLine();
            allowedSet = buildAllowedSet(transps, transpMode, g); 
        }

        cout << "Город отправления: ";
        string fromName = inputLine();
        int from = g.findCity(fromName);
        if (from == -1) {
            cout << "Город не найден.\n";
            return;
        }

        Time startTime = getStartTime();

        if (mode == 4 || mode == 5 || mode == 6) {
            cout << "Город назначения: ";
            string toName = inputLine();
            int to = g.findCity(toName);
            if (to == -1) {
                cout << "Город не найден.\n";
                return;
            }

            Path result;
            if (mode == 4)
                result = findShortestTime(g, from, to, allowedSet, startTime);
            else if (mode == 5)
                result = findCheapest(g, from, to, allowedSet, startTime);
            else if (mode == 6)
                result = findMinTransfers(g, from, to, allowedSet, startTime);

            printPath(result, startTime);
        }
        else if (mode == 7) {
            cout << "Максимальная стоимость: ";
            string lim = inputLine();
            int budget = stoi(lim);
            vector<pair<int, Path>> cities = findWithinBudget(g, from, budget, allowedSet, startTime);
            printCities(cities, startTime, "стоимости", budget);
        }
        else if (mode == 8) {
            cout << "Максимальное время (минуты): ";
            string lim = inputLine();
            int timeLimit = stoi(lim);
            vector<pair<int, Path> > cities = findWithinTime(g, from, timeLimit, allowedSet, startTime);
            printCities(cities, startTime, "времени", timeLimit);
        }
    }

    void printPath(const Path& p, const Time& start) {
        if (p.empty()) {
            cout << "Путь не найден!\n";
            return;
        }
        cout << "\n=== МАРШРУТ ===\n";
        Time cur = start;
        int totalCost = 0;
        int totalWait = 0;
        for (int i = 0; i < p.size(); ++i) {
            int eid = p.edgeId(i);
            const Edge& e = g.getEdge(eid);
            cout << "пересадка " << i+1 << ": "
                 << g.getCityName(e.from) << " -> " << g.getCityName(e.to) << "\n";
            cout << "Транспорт: " << g.getTranspName(e.transport) << "\n";
            cout << "Рейс: " << g.getCruiseName(e.nameId) << "\n";
            cout << "Отправление: " << p.getDeparture(i).fullString() << "\n";
            cout << "Прибытие:   " << p.getArrival(i).fullString() << "\n";
            cout << "Время в пути: " << e.duration << " мин\n";
            cout << "Ожидание: " << p.getWait(i) << " мин\n";
            cout << "Стоимость: " << e.price << "\n\n";
            totalCost += e.price;
            totalWait += p.getWait(i);
            cur = p.getArrival(i);
        }
        cout << "ИТОГО:\n";
        cout << "  Время в пути: " << p.getTotalTime() << " мин\n";
        cout << "  Ожидание: " << totalWait << " мин\n";
        cout << "  Стоимость: " << totalCost << "\n";
        cout << "  Пересадок: " << p.size() << "\n";
    }

    void printCities(const vector<pair<int, Path> >& cities, const Time& start,
                     const string& limitType, int limit)
    {
        if (cities.empty()) {
            cout << "Нет достижимых городов в пределах " << limit << " " << limitType << "\n";
            return;
        }
        cout << "\n=== ГОРОДА В ПРЕДЕЛАХ " << limit << " " << limitType << " ===\n";
        for (size_t i = 0; i < cities.size(); ++i) {
            int city = cities[i].first;
            const Path& p = cities[i].second;
            cout << i+1 << ". " << g.getCityName(city) << "\n";
            cout << "   Время: " << p.getTotalTime() << " мин, цена: " << p.getTotalCost()
                 << ", пересадок: " << p.size() << "\n";
            if (p.size() > 0) {
                cout << "   Прибытие: " << p.getArrival(p.size()-1).fullString() << "\n";
            }
        }
        cout << "Всего: " << cities.size() << "\n";
    }

public:
    void run(int argc, char* argv[]) {
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
#endif
        cout << "ТРАНСПОРТНЫЙ ПОИСКОВИК\n";
        string filename;
        if (argc >= 2) {
            filename = argv[1];
        } else {
            cout << "Введите имя файла ";
            filename = inputLine();
        }

        if (!g.load(filename)) {
            cout << "Ошибка загрузки\n";
            return;
        }
                                                                                                                                             
        cout << "Загружено городов: " << g.cityCount()<< " Типов транспорта: " 
        << g.transpCount()<< " Рейсов: " << g.getAllEdges().size()<< "\n";

        while (true) {
            cout << "\n" << string(55, '*') << "\n";
            cout << "1. Города\n";
            cout << "2. Типы транспорта\n";
            cout << "3. Расписание между 2 городами\n";
            cout << "4. Кратчайший по времени путь\n";
            cout << "5. Самый дешёвый путь\n";
            cout << "6. Минимум пересадок\n";
            cout << "7. Города в пределах бюджета\n";
            cout << "8. Города за заданное время\n";
            cout << "0. Выход\n";
            cout << "Выбор:";
            string choice = inputLine();
            int cmd = stoi(choice);

            switch (cmd) {
                case 1: showCities(); break;
                case 2: showTransport(); break;
                case 3: showSchedule(); break;
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                    search(cmd); break;
                case 0:
                    cout << "Конец\n";
                    return;
                default:
                    cout << "Неверный ввод\n";
            }
        }
    }
};

int main(int argc, char* argv[]) {
    UI ui;
    ui.run(argc, argv);
    return 0;
}