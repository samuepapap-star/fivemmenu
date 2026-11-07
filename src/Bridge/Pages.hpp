#pragma once
#include <Includes/CustomWidgets/Custom.hpp>
#include <Bridge/Features.hpp>
namespace BridgePages {
inline void Footer(){
 BridgeFeatures::Status();
 if(MenuBridge::Connection().Busy()&&Custom::Button("Cancelar acao",ImVec2(200,30),0))MenuBridge::Connection().Cancel();
}
inline void Render(int section){
 ImGui::SetCursorPos(ImVec2(184,136));ImGui::BeginChild("Acoes##Bridge",ImVec2(704,440),false,0);ImGui::PushID(section);
 if(section==0){
  ImGui::TextDisabled("Personagem / modelos e animais");
  static int selected=0;static char name[96]="a_c_chimp";
  const char* models[]={"a_c_chimp","a_c_mtlion","a_c_husky","a_c_cat_01","a_c_boar","a_c_cow","a_c_crow","s_m_y_cop_01","s_m_y_swat_01","mp_m_freemode_01","mp_f_freemode_01"};
  if(ImGui::Combo("Modelo predefinido",&selected,"Chimpanze\0Puma\0Cachorro\0Gato\0Javali\0Vaca\0Corvo\0Policial\0SWAT\0Freemode masculino\0Freemode feminino\0"))MenuBridge::Copy(name,sizeof(name),models[selected]);
  ImGui::InputText("Nome do modelo",name,sizeof(name));
  if(Custom::Button("Transformar personagem",ImVec2(-1,33),0))BridgeFeatures::Model(name);
  if(Custom::Button("Restaurar personagem original",ImVec2(-1,33),0))BridgeFeatures::Restore();
  ImGui::TextWrapped("A aparencia original e guardada antes da primeira troca. Saia do veiculo. A troca desativa God, NoClip e camera livre.");
 }
 if(section==1){
  ImGui::TextDisabled("Personagem / animacoes");
  static char dictionary[96]="missminuteman_1ig_2",animation[96]="handsup_base";static bool loop=true;
  static int selectedAnimation=0;
  const char* dictionaries[]={"missminuteman_1ig_2","anim@mp_player_intcelebrationmale@thumbs_up","anim@mp_player_intcelebrationmale@salute","anim@mp_player_intcelebrationmale@face_palm","amb@world_human_hang_out_street@male_c@idle_a"};
  const char* clips[]={"handsup_base","thumbs_up","salute","face_palm","idle_a"};
  if(ImGui::Combo("Escolher animacao",&selectedAnimation,"Maos ao alto\0Positivo\0Continencia\0Mao no rosto\0Esperar\0")){MenuBridge::Copy(dictionary,sizeof(dictionary),dictionaries[selectedAnimation]);MenuBridge::Copy(animation,sizeof(animation),clips[selectedAnimation]);}
  ImGui::InputText("Dicionario",dictionary,sizeof(dictionary));ImGui::InputText("Animacao",animation,sizeof(animation));Custom::CheckBox("Repetir",&loop);
  if(Custom::Button("Reproduzir animacao",ImVec2(-1,33),0))BridgeFeatures::Animate(dictionary,animation,loop);
  if(Custom::Button("Parar animacao",ImVec2(-1,33),0))BridgeFeatures::StopAnimation();
 }
 if(section==2){
  ImGui::TextDisabled("Carro / gerar veiculo");static char model[96]="adder";static bool network=false,enter=true;
  static int preset=0;const char* models[]={"adder","zentorno","t20","sultan","blista","bati","faggio","bmx"};
  if(ImGui::Combo("Escolher veiculo",&preset,"Adder\0Zentorno\0T20\0Sultan\0Blista\0Bati 801\0Faggio\0BMX\0"))MenuBridge::Copy(model,sizeof(model),models[preset]);
  ImGui::SetNextItemWidth(360);ImGui::InputText("Nome personalizado",model,sizeof(model));
  Custom::CheckBox("Solicitar veiculo em rede",&network);Custom::CheckBox("Entrar no veiculo gerado",&enter);
  if(Custom::Button("Gerar veiculo",ImVec2(-1,33),0))BridgeFeatures::Spawn(model,network,enter);
  ImGui::TextWrapped("Sem rede, o veiculo e local. Em rede, a permanencia e sincronizacao dependem do servidor.");
 }
 Footer();ImGui::PopID();ImGui::EndChild();
}
}

