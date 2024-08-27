pipeline {

    agent none
    stages {
        stage('Run Builds') {
            parallel {
                stage('Ubuntu 20.04 x86-64 (portable)') {
                    agent {
                        label 'Ubuntu_x86-64'
                    }
                    steps {
                        timeout(time: 180, unit: 'MINUTES') {
                            sh '''
                                cp assembly/nix/build-linux-x86-64-nix.sh .
                                chmod +x build-linux-x86-64-nix.sh
                                ./build-linux-x86-64-nix.sh
                            '''
                            sh '''
                                cd artifacts
                                zip -9r ton-x86-64-linux-portable ./*
                            '''
                            archiveArtifacts artifacts: 'artifacts/ton-x86-64-linux-portable.zip'
                        }
                    }
                }
                stage('Ubuntu 20.04 aarch64 (portable)') {
                    agent {
                        label 'Ubuntu_arm64'
                    }
                    steps {
                        timeout(time: 180, unit: 'MINUTES') {
                            sh '''
                                cp assembly/nix/build-linux-arm64-nix.sh .
                                chmod +x build-linux-arm64-nix.sh
                                ./build-linux-arm64-nix.sh
                            '''
                            sh '''
                                cd artifacts
                                zip -9r ton-arm64-linux-portable ./*
                            '''
                            archiveArtifacts artifacts: 'artifacts/ton-arm64-linux-portable.zip'
                        }
                    }
                }
                stage('macOS 12.7 x86-64 (portable)') {
                    agent {
                        label 'macOS_12.7_x86-64'
                    }
                    steps {
                        timeout(time: 180, unit: 'MINUTES') {
                            sh '''
                                cp assembly/nix/build-macos-nix.sh .
                                chmod +x build-macos-nix.sh
                                ./build-macos-nix.sh
                            '''
                            sh '''
                                cd artifacts
                                zip -9r ton-x86-64-macos-portable ./*
                            '''
                            archiveArtifacts artifacts: 'artifacts/ton-x86-64-macos-portable.zip'
                        }
                    }
                }
                stage('macOS 12.6 aarch64 (portable)') {
                    agent {
                        label 'macOS_12.6-arm64-m1'
                    }
                    steps {
                        timeout(time: 180, unit: 'MINUTES') {
                            sh '''
                                cp assembly/nix/build-macos-nix.sh .
                                chmod +x build-macos-nix.sh
                                ./build-macos-nix.sh
                            '''
                            sh '''
                                cd artifacts
                                zip -9r ton-arm64-macos-portable ./*
                            '''
                            archiveArtifacts artifacts: 'artifacts/ton-arm64-macos-portable.zip'
                        }
                    }
                }
                stage('Windows Server 2022 x86-64') {
                    agent {
                        label 'Windows_x86-64'
                    }
                    steps {
                        timeout(time: 180, unit: 'MINUTES') {
                            bat '''
                                copy assembly\\native\\build-windows.bat .
                                build-windows.bat
                            '''
                            bat '''
                                cd artifacts
                                zip -9r ton-x86-64-windows ./*
                            '''
                            archiveArtifacts artifacts: 'artifacts/ton-x86-64-windows.zip'
                        }
                    }
                }
            }
        }
    }
}
