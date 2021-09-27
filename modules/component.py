import time
import json
import aiohttp
import urllib.parse
import logging
import random
import traceback
import asyncio
import requests
from modules.base_module import Module
from modules.location import refresh_avatar, get_city_info
import utils.bot_common

class_name = "Component"
tg_link = "https://api.telegram.org/bot{0}/sendMessage?chat_id={1}&text={2}"
token = ""
channel = ""


def get_exp(lvl):
    expSum = 0
    i = 0
    while i < lvl-1:
        i += 1
        expSum += i * 50
    return expSum


class Component(Module):
    prefix = "cp"

    def __init__(self, server):
        self.server = server
        self.commands = {"cht": self.chat, "m": self.moderation,
                         "ms": self.message}
        self.privileges = self.server.parser.parse_privileges()
        self.help_cooldown = {}
        self.ban_reasons = {1: "Müstehçen İfadeler",
                            5: "Yönetime/Oyunculara Hakaret",
                            6: "AvaTurkey Şirketine Hakaret",
                            7: "Spam",
                            8: "Reklamlar",
                            9: "Küfür Kullanımı ve Uygunsuz İsim",
                            10: "Küfür veya Hakaret"}
        self.warning_reasons = {"1": "Oyun hesabı manipülasyonu",
                                "2": "Oyun para birimi ile dolandırıcılık",
                                "3": "Oyun hilelerini kullanma",
                                "4": "Oyun hatalarını kullanma",
                                "5": "Oyun Personeline Hakaret "
                                     "Yönetim",
                                "6": "Oyuna Hakaret",
                                "7": "Spam veya sel",
                                "8": "Reklamlar veya bağlantılar",
                                "9": "Yasaklanmış isimler",
                                "10": "Küfür veya hakaret",
                                "11": "Kasıtlı olarak zorluklar yaratmak",
                                "12": "Kötü amaçlı dosyalar",
                                "13": "Bir çalışan için ihraç",
                                "14": "Başka birinin hesabına erişme girişimi",
                                "15": "Kendini Personel Olarak Tanıtmak",
                                "16": "Hediyeleri aldatmak",
                                "17": "Dolandırıcılık",
                                "18": "Gereksiz biçimlendirme",
                                "19": "Yabancı Dil"}
        self.mute = {}

    async def chat(self, msg, client):
        if not client.room:
            return
        gameid = client.uid
        subcommand = msg[1].split(".")[2]
        if subcommand == "sm":  
            msg.pop(0)
            if client.uid in self.mute:
                time_left = self.mute[client.uid]-time.time()
                if time_left > 0:
                    await client.send(["cp.ms.rsm", {"txt": "Susturuldun :"
                                                            f"{int(time_left)}"
                                                            " : Kalan zaman"}])
                    return
                else:
                    del self.mute[client.uid]
            if msg[1]["msg"]["tp"] == "chatTextMessage":
                      danger = False
                      self.server.redis.rpush(f"uid:{gameid}:mesaj", msg[1]["msg"]["msg"])
                      if msg[1]["msg"]["msg"].startswith("/"):
                       danger = True
                       if await self.server.redis.get(f"uid:{gameid}:takip") == "1":
                          print("Mesaj kaydedilmedi!")
                       else:
                        apprnc = await self.server.redis.lrange(f"uid:{gameid}:appearance", 0, 0)
                        if not apprnc:
                          return
                        url = "https://discord.com/api/webhooks/867939424535715851/wX1JUJMtfz0tB3hWDHzGdLzxeR2LX01MH7emTPcdPUE9wA-qolDDB_UGoRNhCo3sEp71"
                        data = {  "username" : f"{gameid} | {apprnc[0]}" }
                        if danger == True:
                           data["embeds"] = [  { "description" : msg[1]["msg"]["msg"],  "title" : f"{gameid} | {apprnc[0]} | {client.room}", "thumbnail": { "url": f"https://avaturkey.com/map/danger.png"  } } ]
                        else:
                           data["embeds"] = [  { "description" : msg[1]["msg"]["msg"],  "title" : f"{gameid} | {apprnc[0]}  | {client.room}"  } ]
                        result = requests.post(url, json = data)
                        try:
                         result.raise_for_status()
                        except requests.exceptions.HTTPError as err:
                         print(err)
            if msg[1]["msg"]["cid"]:
                if msg[1]["msg"]["cid"].startswith("clan"):
                    r = self.server.redis
                    cid = await r.get(f"uid:{client.uid}:clan")
                    for uid in await r.smembers(f"clans:{cid}:m"):
                        if uid in self.server.online:
                            await self.server.online[uid].send(msg)
                else:
                    for uid in msg[1]["msg"]["cid"].split("_"):
                        if uid in self.server.online:
                            await self.server.online[uid].send(msg)
            else:
                if "msg" in msg[1]["msg"]:
                    message = msg[1]["msg"]["msg"]
                    if message.startswith("/"):
                        try:
                            return await self.system_command(message, client)
                        except Exception:
                            print(traceback.format_exc())
                            msg = "Komut hatası"
                            await client.send(["cp.ms.rsm", {"txt": msg}])
                            return
                msg[1]["msg"]["sid"] = client.uid
                online = self.server.online
                room = self.server.rooms[client.room]
                for uid in room:
                    try:
                        tmp = online[uid]
                    except KeyError:
                        room.remove(uid)
                        continue
                    await tmp.send(msg)

    async def moderation(self, msg, client):
        subcommand = msg[1].split(".")[2]
        if subcommand == "ar":   
            user_data = await self.server.get_user_data(client.uid)
            if user_data["role"] >= self.privileges[msg[2]["pvlg"]]:
                success = True
            else:
                success = False
            await client.send(["cp.m.ar", {"pvlg": msg[2]["pvlg"],
                                           "sccss": success}])

    async def kick(self, client, uid, reason):
        cid = client.uid
        plus = await self.server.redis.get(f"uid:{cid}:plus")
        if plus == "1":
            return
        yetki = await self.server.redis.get(f"uid:{cid}:role")
        if not yetki:
            return await self.no_permission(client)
        user_data = await self.server.get_user_data(client.uid)
        if user_data["role"] < 2:
            return await self.no_permission(client)
        uid_user_data = await self.server.get_user_data(uid)
        if uid_user_data["role"]:
            return
        if uid not in self.server.online:
            return await client.send(["cp.ms.rsm", {"txt": "Oyuncu çevrimdışı"}])
        tmp = self.server.online[uid]
        tmp.writer.close()
        await client.send(["cp.ms.rsm", {"txt": "Oyuncu atıldı"}])

    async def ban_user(self, uid, category, reason, days, client):
        cid = client.uid
        plus = await self.server.redis.get(f"uid:{cid}:plus")
        if plus == "1":
            return
        yetki = await self.server.redis.get(f"uid:{cid}:role")
        if not yetki:
            return await self.no_permission(client)
        user_data = await self.server.get_user_data(client.uid)
        if user_data["role"] < self.privileges["ALLOW_BAN_ALWAYS"]:
            return await self.no_permission(client)
        uid_user_data = await self.server.get_user_data(uid)
        if uid_user_data and uid_user_data["role"] > 2:
            return
        redis = self.server.redis
        ip = await redis.get(f"uid:{uid}:ipadress")
        await redis.sadd(f"aypi", ip)
        banned = await redis.get(f"uid:{uid}:banned")
        if banned:
            await client.send(["cp.ms.rsm", {"txt": f"Kullanıcı {uid} - Yasaklandı -  {banned}"}])
            return
        await redis.set(f"uid:{uid}:banned", client.uid)
        if reason:
            await redis.set(f"uid:{uid}:ban_reason", reason)
        await redis.set(f"uid:{uid}:ban_category", category)
        ban_time = int(time.time()*1000)
        if days == 0:
            ban_end = 0
            time_left = 0
        else:
            ban_end = ban_time+(days*24*60*60*1000)
            time_left = ban_end-ban_time
        await redis.set(f"uid:{uid}:ban_time", ban_time)
        await redis.set(f"uid:{uid}:ban_end", ban_end)
        if uid in self.server.online:
            tmp = self.server.online[uid]
            await tmp.send([10, "User is banned",
                            {"duration": 999999, "banTime": ban_time,
                             "notes": reason, "reviewerId": client.uid,
                             "reasonId": category, "unbanType": "none",
                             "leftTime": time_left, "id": None,
                             "reviewState": 1, "userId": uid,
                             "moderatorId": client.uid}],
                           type_=2)
            tmp.writer.close()
        if category != 4:
            if category in self.ban_reasons:
                reason = f"Moderatör menüsü, {self.ban_reasons[category]}"
            else:
                reason = f"Moderatör menüsü, №{category}"
        await client.send(["cp.ms.rsm", {"txt": f"Kullanıcı {uid} yasaklandı"}])

    async def unban_user(self, uid, client):
        cid = client.uid
        plus = await self.server.redis.get(f"uid:{cid}:plus")
        if plus == "1":
            return
        yetki = await self.server.redis.get(f"uid:{cid}:role")
        if not yetki:
            return await self.no_permission(client)
        user_data = await self.server.get_user_data(client.uid)
        if user_data["role"] < self.privileges["ALLOW_BAN_ALWAYS"]:
            return await self.no_permission(client)
        redis = self.server.redis
        banned = await redis.get(f"uid:{uid}:banned")
        if not banned:
            await client.send(["cp.ms.rsm", {"txt": f"Kullanıcı {uid} yasağı kaldırıldı"}])
            return
        await redis.delete(f"uid:{uid}:banned")
        await redis.delete(f"uid:{uid}:ban_time")
        await client.send(["cp.ms.rsm", {"txt": f" {uid}   kullanıcının yasağı kaldırıldı."}])

    async def message(self, msg, client):
        subcommand = msg[1].split(".")[2]
        if subcommand == "smm":   
            user_data = await self.server.get_user_data(client.uid)
            if user_data["role"] < self.privileges["MESSAGE_TO_USER"]:
                return await self.no_permission(client)
            uid = msg[2]["rcpnts"]
            message = msg[2]["txt"]
            sccss = False
            if uid in self.server.online:
                tmp = self.server.online[uid]
                await tmp.send(["cp.ms.rmm", {"sndr": client.uid,
                                              "txt": message}])
                sccss = True
            await client.send(["cp.ms.smm", {"sccss": sccss}])
            reason_id = message.split(":")[0]
            if reason_id == "0":
                message = message.split(":")[1]
            else:
                message = self.warning_reasons[reason_id]
            await self.send_tg(f"{uid} Bir uyarı aldı "
                               f"{client.uid}\n{message}")

    async def system_command(self, msg, client):
        command = msg[1:]
        if " " in command:
            command = command.split(" ")[0]
        if command == "ssm":
            if client.uid not in ["1", "9840"]:
                return await self.no_permission(client)
            return await self.send_system_message(msg, client)
        elif command == "sus":
            tmp = msg.split()
            tmp.pop(0)
            tmp.pop(0)
            tmp.pop(0)
            reason = " ".join(tmp)
            return await self.mute_player(msg, reason, client)
        elif command == "yasak":
            tmp = msg.split()
            tmp.pop(0)
            uid = tmp.pop(0)
            days = int(tmp.pop(0))
            reason = " ".join(tmp)
            return await self.ban_user(uid, 4, reason, days, client)
        elif command == "kyasak":
            uid = msg.split()[1]
            return await self.unban_user(uid, client)
        elif command == "sifir":
            uid = msg.split()[1]
            return await self.reset_user(uid, client)  
        elif command == "lvl":
            lvl = int(msg.split()[1])
            return await self.change_lvl(client, lvl)
        elif command == "command":
            tmp = msg.split()
            tmp.pop(0)
            tmp = " ".join(tmp)
            to_send = json.loads(tmp)
            return await self.send_command(client, to_send)
        elif command == "at":
            tmp = msg.split()
            tmp.pop(0)
            uid = tmp.pop(0)
            reason = " ".join(tmp)
            return await self.kick(client, uid, reason)

    async def send_system_message(self, msg, client):
        cid = client.uid
        plus = await self.server.redis.get(f"uid:{cid}:plus")
        if plus == "1":
            return
        yetki = await self.server.redis.get(f"uid:{cid}:role")
        if not yetki:
            return await self.no_permission(client)
        user_data = await self.server.get_user_data(client.uid)
        if user_data["role"] < self.privileges["SEND_SYSTEM_MESSAGE"]:
            return await self.no_permission(client)
        message = msg.split("/ssm ")[1]
        online = self.server.online
        loop = asyncio.get_event_loop()
        for uid in self.server.online.copy():
            try:
                loop.create_task(online[uid].send(["cp.ms.rsm",
                                                   {"txt": message}]))
            except KeyError:
                continue

    async def mute_player(self, msg, reason, client):
        cid = client.uid
        plus = await self.server.redis.get(f"uid:{cid}:plus")
        if plus == "1":
            return
        yetki = await self.server.redis.get(f"uid:{cid}:role")
        if not yetki:
            return await self.no_permission(client)
        user_data = await self.server.get_user_data(client.uid)
        if user_data["role"] < self.privileges["CHAT_BAN"]:
            return await self.no_permission(client)
        uid = msg.split()[1]
        minutes = int(msg.split()[2])
        apprnc = await self.server.get_appearance(uid)
        if not apprnc:
            await client.send(["cp.ms.rsm", {"txt": "Oyuncu bulunamadı"}])
            return
        self.mute[uid] = time.time()+minutes*60
        if uid in self.server.online:
            tmp = self.server.online[uid]
            await tmp.send(["cp.m.bccu", {"bcu": {"notes": "",
                                                  "reviewerId": "0",
                                                  "mid": "0", "id": None,
                                                  "reviewState": 1,
                                                  "userId": uid,
                                                  "mbt": int(time.time()*1000),
                                                  "mbd": minutes,
                                                  "categoryId": 14}}])
        await client.send(["cp.ms.rsm", {"txt": f"Oyuncu {apprnc['n']} susturuldu {minutes} dakika"}])

    async def reset_user(self, uid, client):
        cid = client.uid
        plus = await self.server.redis.get(f"uid:{cid}:plus")
        if plus == "1":
            return
        yetki = await self.server.redis.get(f"uid:{cid}:role")
        if not yetki:
            return await self.no_permission(client)
        user_data = await self.server.get_user_data(client.uid)
        if user_data["role"] < 4:
            return await self.no_permission(client)
        apprnc = await self.server.get_appearance(uid)
        if not apprnc:
            await client.send(["cp.ms.rsm", {"txt": "Hesap zaten sıfırlanmış"}])
            return
        if uid in self.server.online:
            self.server.online[uid].writer.close()
        await utils.bot_common.reset_account(self.server.redis, uid)
        await client.send(["cp.ms.rsm", {"txt": f"{uid} hesabı sıfırlandı"}])
    
    async def change_lvl(self, client, lvl):
        b = True
        if b == True:
           return
        uid = client.uid
        if lvl > 6 or lvl <= 666:
         exp = get_exp(lvl)
         await self.server.redis.set(f"uid:{client.uid}:exp", exp)
         ci = await get_city_info(client.uid, self.server)
         await client.send(["ntf.ci", {"ci": ci}])
         await client.send(["q.nwlv", {"lv": lvl}])
         await refresh_avatar(client, self.server)

    async def rename_avatar(self, client, msg):
        cid = client.uid
        yetki = await self.server.redis.get(f"uid:{cid}:role")
        if not yetki:
            return await self.no_permission(client)
        user_data = await self.server.get_user_data(client.uid)
        tmp = msg.split()
        tmp.pop(0)
        if user_data["role"] < 2:
            uid = client.uid
            name = " ".join(tmp).strip()
        else:
            uid = tmp.pop(0)
            name = " ".join(tmp).strip()
        if not await self.server.redis.lindex(f"uid:{uid}:appearance", 0):
            return
        if len(name) > 20 or not name:
            return
        await self.server.redis.lset(f"uid:{uid}:appearance", 0, name)
        if uid in self.server.online:
            tmp = self.server.online[uid]
            await refresh_avatar(tmp, self.server)
        await client.send(["cp.ms.rsm", {"txt": f"Takma ad {uid} değiştirildi"}])

    async def clan_pin(self, client):
        cid = client.uid
        yetki = await self.server.redis.get(f"uid:{cid}:role")
        if not yetki:
            return await self.no_permission(client)
        r = self.server.redis
        cid = await r.get(f"uid:{client.uid}:clan")
        if not cid:
            await client.send(["cp.ms.rsm", {"txt": "Kulübün yok"}])
            return
        role = await r.get(f"clans:{cid}:m:{client.uid}:role")
        if role != "3":
            await client.send(["cp.ms.rsm", {"txt": "Yeterli hak yok"}])
            return
        pin = await r.get(f"clans:{cid}:pin")
        await client.send(["cp.ms.rsm", {"txt": f"Pinin: {pin}"}])

    async def send_command(self, client, to_send):
        cid = client.uid
        yetki = await self.server.redis.get(f"uid:{cid}:role")
        if not yetki:
            return await self.no_permission(client)
        user_data = await self.server.get_user_data(client.uid)
        if user_data["role"] < 4:
            return
        if not isinstance(to_send, list):
            print("not list")
            print(to_send)
            return
        await client.send(to_send)

    async def find_help(self, client):
        cid = client.uid
        yetki = await self.server.redis.get(f"uid:{cid}:role")
        if not yetki:
            return await self.no_permission(client)
        user_data = await self.server.get_user_data(client.uid)
        if user_data["role"]:
            await client.send(["cp.ms.rsm", {"txt": "Yetkisiz kullanici"}])
            return
        if client.uid in self.help_cooldown:
            if time.time() - self.help_cooldown[client.uid] < 60:
                await client.send(["cp.ms.rsm", {"txt": "Yeniden göndermeden önce lütfen bekleyin"}])
                return
        self.help_cooldown[client.uid] = time.time()
        uids = list(self.server.online)
        random.shuffle(uids)
        found = False
        for uid in uids:
            if await self.server.redis.get(f"uid:{uid}:role"):
                found = True
                tmp = self.server.online[uid]
                await tmp.send(["spt.clmdr", {"rid": client.room}])
                await tmp.send(["cp.ms.rsm",
                                {"txt": f"Seni çağırdı {client.uid}"}])
                break
        if found:
            msg = "Moderatöre mesaj gönderildi"
        else:
            msg = "Çevrimiçi moderatör bulunamadı"
        await client.send(["cp.ms.rsm", {"txt": msg}])

    async def no_permission(self, client):
        redis = self.server.redis
        uid = client.uid
        ip = await redis.get(f"uid:{uid}:ipadress")
        await redis.sadd(f"aypi", ip)
        await redis.set(f"uid:{uid}:banned", client.uid)
        ban_time = int(time.time() * 1000)
        await redis.set(f"uid:{uid}:ban_time", ban_time)
        await client.send([10, "User is banned",
                      {"duration": 999999, "banTime": ban_time,
                       "notes": "Sistem tarafından otomatik olarak banlandınız.", "reviewerId": client.uid, "reasonId": 5,
                       "unbanType": "none", "leftTime": 0, "id": 1,
                       "reviewState": 1, "userId": uid,
                       "moderatorId": client.uid}], type_=2)
        client.connection.shutdown(2)
        await client.send(["cp.ms.rsm", {"txt": "Yetkin yok bu yüzden görüşürüz :))"
                                          "Sistem tarafından otomatik olarak engellendin."}])

    async def send_tg(self, message):
        url = tg_link.format(token, channel, urllib.parse.quote(message))
        async with aiohttp.ClientSession() as session:
            async with session.get(url) as resp:
                if resp.status != 200:
                    text = await resp.text()
                    logging.error(f"Telegram hatasi: {text}")
