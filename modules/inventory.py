import random
from modules.base_module import Module
from modules.location import get_city_info
import modules.notify as notify
import const
import json
import logging

with open("./box/erkek.json", "r") as f:
    esyalar_erkek = json.load(f)

with open("./box/kiz.json", "r") as f:
    esyalar_kadin = json.load(f)

class_name = "Inventory"


class Inventory(Module):
    prefix = "tr"

    def __init__(self, server):
        self.server = server
        self.commands = {"sale": self.sale_item, "opgft": self.open_gift}

    async def sale_item(self, msg, client):
        items = self.server.game_items["game"]
        tpid = msg[2]["tpid"]
        cnt = msg[2]["cnt"]
        if tpid not in items or "saleSilver" not in items[tpid]:
            return
        if not await self.server.inv[client.uid].take_item(tpid, cnt):
            return
        price = items[tpid]["saleSilver"]
        user_data = await self.server.get_user_data(client.uid)
        redis = self.server.redis
        await redis.set(f"uid:{client.uid}:slvr", user_data["slvr"]+price*cnt)
        ci = await get_city_info(client.uid, self.server)
        await client.send(["ntf.ci", {"ci": ci}])
        inv = self.server.inv[client.uid].get()
        await client.send(["ntf.inv", {"inv": inv}])
        await notify.update_resources(client, self.server)

    async def open_gift(self, msg, client):
        tpid = msg[2]["tpid"]
        act = await self.server.redis.get(f"uid:{client.uid}:act")
        if act != None:
            sonuc = int(act) + 5
            await self.server.redis.set(f"uid:{client.uid}:act", sonuc)
        else:
            sonuc = 5
            await self.server.redis.set(f"uid:{client.uid}:act", sonuc)
        userApprnc = await self.server.get_appearance(client.uid)
        if userApprnc["g"] == 1:
            esyalar = esyalar_erkek
        if userApprnc["g"] == 2:
            esyalar = esyalar_kadin

        if (tpid not in esyalar):
            return
        
        if not self.server.inv[client.uid].take_item(tpid, 1):
            return

        gift = esyalar[tpid]

        res = {
            "gld": 0,
            "slvr": 0,
            "enrg": 0
        }

        

        count = await self.server.inv[client.uid].get_item(tpid)

        await client.send([
            "ntf.inv",
            {
                "it": {
                    "c": count,
                    "iid": "",
                    "tid": tpid
                }
            }
        ])

        if ("silver" in gift):
            res["slvr"] = random.randint(gift["silver"][0], gift["silver"][1])

        if ("gold" in gift):
            res["gld"] = random.randint(gift["gold"][0], gift["gold"][1])

        if ("energy" in gift):
            res["enrg"] = random.randint(gift["energy"][0], gift["energy"][1])

        winItems = []

        for item in gift["items"]:
            giftItems = []

            id = item["id"]
            it = item["it"]

            for loot in it:
                if ("gender" in item):
                    gender = "girl"
                    
                    if (userApprnc["g"] == 2):
                        gender = "boy"

                    if (gender != item["gender"]):
                        continue

                giftItems.append(loot)

            winItem = random.choice(giftItems)

            winItems.append({
                "tid": winItem["esya_ismi"],
                "iid": "",
                "c": random.randint(winItem["en_az"], winItem["en_fazla"]),
                "atr": { "bt": 0 },
                "id": id
            })

        await client.send([
            "tr.opgft",
            {
                "lt": { "id": "lt", "it": winItems },
                "res": res,
                "ctid": "personalGifts"
            }
        ])

        userData = await self.server.get_user_data(client.uid)
 
        await self.server.redis.set(f"uid:{client.uid}:enrg", userData["enrg"] + res["enrg"])
        await self.server.redis.set(f"uid:{client.uid}:slvr", userData["slvr"] + res["slvr"])

        userData = await self.server.get_user_data(client.uid)

        await client.send([
            "ntf.res",
            {
                "res": {
                    "gld": userData["gld"],
                    "slvr": userData["slvr"],
                    "enrg": userData["enrg"]
                }
            }
        ])

        for item in winItems:
            logging.error(item)
            

        await client.send([
            "ntf.inv",
            {
                "inv": self.server.inv[client.uid].inv
            }
        ])
