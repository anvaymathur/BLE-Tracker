import { initializeApp } from "firebase/app";
import { getDatabase, ref, get, child, onValue} from "firebase/database";

const firebaseConfig = {
  apiKey: import.meta.env.VITE_FIREBASE_API_KEY,
  authDomain: import.meta.env.VITE_FIREBASE_AUTH_DOMAIN,
  databaseURL: import.meta.env.VITE_FIREBASE_DATABASE_URL,
//   projectId: import.meta.env.VITE_FIREBASE_PROJECT_ID,
  storageBucket: import.meta.env.VITE_FIREBASE_STORAGE_BUCKET,
  messagingSenderId: import.meta.env.VITE_FIREBASE_MESSAGING_SENDER_ID,
//   appId: import.meta.env.VITE_FIREBASE_APP_ID
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const db = getDatabase(app);

export function watchGateways(callback){
    const dbRef = ref(db);
    return onValue(dbRef, (snapshot) => {
        if (snapshot.exists()){
            callback(snapshot.val());
        } else {
            callback(null);
        }
    }, (e) => {
        console.error(`Could not detect any gateways: `, e);
    })
}

export async function getGatewayTags(gatewayID){
    const dbRef = ref(db);
    try {
        const snapshot = await get(child(dbRef,gatewayID));

        if (snapshot.exists()){
            return snapshot.val();
        } else {
            console.log("No data available at this gateway id");
        }
    } catch (e){
        console.error("Error retreiving gateway information", e);
    }     
}

export function watchGatewayTags(gatewayID, callback){
    const dbRef = ref(db,gatewayID);    
    return onValue(dbRef, (snapshot) => {
        if (snapshot.exists()){
            callback(snapshot.val());
        } else {
            callback(null);
        }
    }, (e) => {
        console.error(`Could not detect gateway with gateway id: ${gatewayID}`, e);
    }); 
}

export async function getTagRSSI(gatewayID, tagMAC){
    const dbRef = ref(db);
    try {
        const snapshot = await get(child(dbRef, `${gatewayID}/${tagMAC}/rssi`));

        if (snapshot.exists()){
            return snapshot.val();
        } else {
            console.log("No tag rssi found");
        }
    } catch (e){
        console.error("Error retreiving tag rssi", e);
    }
}

export function watchTagRSSI(gatewayID, tagMAC, callback){
    const dbRef = ref(db,`${gatewayID}/${tagMAC}/rssi`);    

    return onValue(dbRef, (snapshot) => {
        if (snapshot.exists()){
            callback(snapshot.val());
        } else {
            callback(null);
        }
    }, (e) => {
        console.error(`Could not detect rssi of tag ${tagMAC} with gateway id: ${gatewayID}`, e);
    }); 
}

export async function getTagLastSeen(gatewayID, tagMAC){
    const dbRef = ref(db);
    try {
        const snapshot = await get(child(dbRef, `${gatewayID}/${tagMAC}/last_seen`));

        if (snapshot.exists()){
            return snapshot.val();
        } else {
            console.log("No tag last seen found");
        }
    } catch (e){
        console.error("Error retreiving tag last seen", e);
    }
}

export function watchTagLastSeen(gatewayID, tagMAC, callback){
    const dbRef = ref(db,`${gatewayID}/${tagMAC}/last_seen`);    

    return onValue(dbRef, (snapshot) => {
        if (snapshot.exists()){
            callback(snapshot.val());
        } else {
            callback(null);
        }
    }, (e) => {
        console.error(`Could not detect last_seen of tag ${tagMAC} with gateway id: ${gatewayID}`, e);
    }); 
}