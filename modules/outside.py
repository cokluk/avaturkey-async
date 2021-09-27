from modules.location import Location, gen_plr, get_cc
import const
import logging
import requests
from random import randrange
import random
class_name = "Outside"


couturier_botlari = []

class Outside(Location):
    prefix = "o"

    def __init__(self, server):
        super().__init__(server)
        self.commands.update({"r": self.room, "gr": self.get_room})

    async def disconnect(self, prefix, tmp, client):
        await tmp.send([prefix+".r.lv", {"uid": client.uid}])
        await tmp.send([client.room, client.uid], type_=17)

    async def connect(self, plr, tmp, client):
        await tmp.send(["o.r.jn", {"plr": plr}])
        await tmp.send([client.room, client.uid], type_=16)

    async def get_room(self, msg, client):
        botsayi = int(await self.server.redis.get("bot"))
        if botsayi:
            for i in range(botsayi):
                 b1_id = randrange(int(await self.server.redis.get("uids")))
                 if await self.server.redis.get(f"uid:{b1_id}:hrt") and await self.server.redis.get(f"uid:{b1_id}:crt"):
                  b1_hrt = int(await self.server.redis.get(f"uid:{b1_id}:hrt"))
                  b1_crt = int(await self.server.redis.get(f"uid:{b1_id}:crt"))
                  b1_exp = int(await self.server.redis.get(f"uid:{b1_id}:exp"))
                  b1_rpt = int(await self.server.redis.get(f"uid:{b1_id}:rpt"))
                  if not b1_rpt:
                     b1_rpt = -600
                  clothes = []
                  b1_x = random.randrange(1, 10)+.1+(i/2)*2.5
                  b1_y = random.randrange(1, 10)+.1+(i/2)*2.5
                  cur_ctp = await self.server.redis.get(f"uid:{b1_id}:wearing")
                  for item in await self.server.redis.smembers(f"uid:{b1_id}:{cur_ctp}"):
                    if "_" in item:
                     id_, clid = item.split("_")
                     clothes.append({'tpid': id_, 'clid': clid})
                    else: 
                     clothes.append({'tpid': item, 'clid': ""})
        
                  apprnc = await self.server.redis.lrange(f"uid:{b1_id}:appearance", 0, -1)
                  if not b1_exp:
                     b1_exp = 1
                  if apprnc and clothes and int(b1_exp) > 5 and int(b1_crt) > 251000:
                   b1 = { 'uid': b1_id, 'apprnc': {'n': apprnc[0], 'g': int(apprnc[2]), 'nct': int(apprnc[1]),
                "sc": int(apprnc[3]), "ht": int(apprnc[4]),
                "hc": int(apprnc[5]), "brt": int(apprnc[6]),
                "brc": int(apprnc[7]), "et": int(apprnc[8]),
                "ec": int(apprnc[9]), "fft": int(apprnc[10]),
                "fat": int(apprnc[11]), "fac": int(apprnc[12]),
                "ss": int(apprnc[13]), "ssc": int(apprnc[14]),
                "mt": int(apprnc[15]), "mc": int(apprnc[16]),
                "sh": int(apprnc[17]), "shc": int(apprnc[18]),
                "rg": int(apprnc[19]), "rc": int(apprnc[20]),
                "pt": int(apprnc[21]), "pc": int(apprnc[22]),
                "bt": int(apprnc[23]), "bc": int(apprnc[24]) }, 'clths': {'clths': clothes }, 'mbm': {'ac': 'mouseEarsMobileAccessory', 'sk': None, 'rt': 'mbRgn8', 'nmc': 1}, 'usrinf': {'rl': 0, 'sid': b1_id }, 'chtdcm': {'bdc': None, 'tcl': None, 'spks':  None }, 'locinfo': {'st': 0, 's': '127.0.0.1', 'at': '', 'd': 4, 'x': b1_x	, 'y':  b1_y, 'shlc': True, 'pl': '', 'l': 'couturier_ctr1_1'}, 'clif': None, 'ci': {'exp': b1_exp, 'crt': b1_crt, 'hrt': b1_hrt, 'fexp': 0, 'gdc': 0, 'lgt': 0, 'vip': True, 'vexp': 99999, 'vsexp': 99999, 'vsact': True, 'vret': 0, 'vfgc': 0, 'ceid': 0, 'cmid': 0, 'dr': True, 'spp': 0, 'tts': None, 'eml': None, 'ys': 0, 'ysct': 0, 'fak': None, 'shcr': True, 'gtrfrd': 0, 'strfrd': 0, 'rtrtm': 0, 'kyktid': None, 'actrt': 26511, 'compid': 0, 'actrp': 1, 'actrd': 1899999999, 'shousd': False, 'rpt': 600, 'as': None, 'lvt': 1617950465, 'lrnt': 0, 'lwts': 0, 'skid': 'iceRinkSkate1', 'skrt': 1627950465, 'bcld': 0, 'trid': None, 'trcd': 0, 'sbid': None, 'sbrt': 0, 'plcmt': {'pc': {'snowboardRating': {'uid': 0, 'ct': -500, 'cid': 812, 'cr': 55000}}}, 'pamns': {'amn': []}, 'crst': 0, 'psrtdcr': 12, 'dl': True}, 'pf': {'pf': {'jntr': {'tp': 'jntr', 'l': 20, 'pgs': 0}, 'phtghr': {'tp': 'phtghr', 'l': 20, 'pgs': 0}, 'grdnr': {'tp': 'grdnr', 'l': 20, 'pgs': 0}, 'vsgst': {'tp': 'vsgst', 'l': 20, 'pgs': 0}}}}
                   couturier_botlari.append(b1)
        uid = client.uid
        if await self.server.redis.get(f"uid:{uid}:lang") == "tr":
            sayi = 1
        else:
            sayi = 10
        if "rid" not in msg[2]:
            num = sayi
            while True:
                room = f"{msg[2]['lid']}_{msg[2]['gid']}_{num}"
                if self._get_room_len(room) >= const.ROOM_LIMIT:
                    num += sayi
                else:
                    break
        else:
            room = f"{msg[2]['lid']}_{msg[2]['gid']}_{msg[2]['rid']}"
        if client.room:
            await self.leave_room(client)
        logging.error(room)
        await self.server.redis.set(f"uid:{uid}:roomx", room)
        await self.join_room(client, room)
        await client.send(["o.gr", {"rid": client.room}])

    async def room(self, msg, client):
        subcommand = msg[1].split(".")[2]
        if subcommand == "info":
            rmmb = []
            room = self.server.rooms[msg[0]].copy()
            online = self.server.online
            location_name = msg[0].split("_")[0]
            cl = {"l": {"l1": [], "l2": []}}
            for uid in room:
                if uid not in online:
                    if uid in self.server.rooms[msg[0]]:
                        self.server.rooms[msg[0]].remove(uid)
                    continue
                rmmb.append(await gen_plr(online[uid], self.server))
                for element in couturier_botlari:
                     rmmb.append(element)
                if location_name == "canyon":
                    cl["l"][online[uid].canyon_lid].append(uid)
            if location_name == "canyon":
                cc = await get_cc(msg[0], self.server)
            else:
                cc = None
                cl = None
            uid = msg[0].split("_")[-1]
            evn = None
            if uid in self.server.modules["ev"].events:
                event = await self.server.modules["ev"]._get_event(uid)
                event_room = f"{event['l']}_{uid}"
                if event_room == msg[0]:
                    evn = event
            await client.send(["o.r.info", {"rmmb": rmmb, "evn": evn,
                                            "cc": cc, "cl": cl}])
        else:
            await super().room(msg, client)
        couturier_botlari.clear()

    def _get_room_len(self, room):
        if room not in self.server.rooms:
            return 0
        return len(self.server.rooms[room])
